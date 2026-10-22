# How the N64 game runs on the Wii

This port is not an emulator. The decompiled game (`src/`, `lib/libmus`) is compiled for the Wii's PowerPC CPU with
`-DTARGET_WII -DVERSION_US`; both machines are big-endian 32-bit, so structs, display lists and asset data keep their
N64 layout. What changes is everything *below* the game: libultra (`lib/ultralib`) is not built for the Wii, and
`port/` re-implements the services the game expects from the N64 hardware and OS on top of libogc. Changes inside
`src/` are behind `#ifdef TARGET_WII`, and the N64 build still matches byte for byte (`make` prints `: OK`).

Overview:

| N64 | Wii port |
|---|---|
| libultra threads, message queues, events | `port/ultra/os_sched.c`, `port/ultra/os_events.c` |
| VI retrace interrupt | libogc post-retrace callback -> event pump |
| `osGetTime` / `osSetTimer` | `os_events.c` (timer LWP, CPU-counter emulation) |
| PI DMA from the cartridge | `memcpy` from the ROM embedded in the DOL (`os_stub.c`) |
| Controllers (SI) | `port/wii/pad_wii.c` (Wiimote / Classic / GameCube) |
| EEPROM | `port/ultra/os_eeprom.c` (file on the SD card) |
| RSP graphics microcode (F3DEX2/S2DEX) + RDP | display-list interpreter that draws with GX (`port/gfx/`) |
| RSP audio microcode | C interpreter of the audio command list (`port/audio/naudio.c`) |
| AI (audio interface) | Wii AI DMA (`AUDIO_*`, `os_stub.c`) |
| Framebuffer + VI output | EFB at 2x (640x480) -> XFB (`port/wii/main.c`) |

## Threads and the scheduler

The game creates libultra threads (idle, main, the NN scheduler's event/audio/graphics threads, the graphics thread,
the libmus music thread, a background-task thread) with N64 priorities. `port/ultra/os_sched.c` gives each one a
libogc LWP with its own 64 KB stack (PowerPC stack frames are larger than MIPS ones) and runs them **cooperatively**:
exactly one game thread holds the CPU at a time, in strict N64 priority order. A thread gives up the CPU when it
blocks (`osRecvMesg` on an empty queue, `osSendMesg` on a full one, `osStopThread`, ...) or at explicit preemption
points (`port_preempt_check`, e.g. in `osGetTime`). This reproduces libultra's semantics - the game's code relies on
them - instead of letting libogc's own scheduler interleave the threads freely.

Helper LWPs that are not game threads: the event pump, the timer LWP, the "RSP" LWP (owner of the GX FIFO) and the
SD-log flusher. They never take the game-thread CPU token; they hand work to game threads through message queues.

Differences from the N64 worth knowing:

- An interrupt on the N64 runs its handler immediately and wakes the receiving thread; in the port, a woken
  higher-priority thread runs at the next scheduling point of the current thread. Queues therefore fill up more than
  on real hardware.
- `osSetIntMask` only saves/restores the mask; it never switches threads (the game wraps short critical sections in
  it and a switch there broke them).

## Events and message queues

`osCreateMesgQueue` / `osSendMesg` / `osRecvMesg` / `osJamMesg` are implemented in `os_sched.c` with a waiter table per
queue. Hardware events (`osViSetEvent`, `osSetEventMesg` for SP/DP/SI/PRENMI, timers) are produced by interrupt
callbacks and helper LWPs, put into a small ring (`port_post_event`, IRQ-safe), and delivered to the game's queues by
the **event pump** LWP (`port_deliver_event`).

**Events are never dropped.** libultra posts events with `OS_MESG_NOBLOCK`, which silently drops a message when the
queue is full. That is harmless on the N64 because the receiver runs at once. In the port queues do fill (see above),
and a dropped "graphics task done" or timer message left a thread waiting forever: a frozen picture. The pump now keeps
undeliverable events in a per-queue pending list and delivers them, in order, as soon as the receiver takes a message
(`osRecvMesg` path). A once-per-second `ev/sec: post=.. ok=.. drop=..` log line must show `drop=0`.

## VI retrace, frame counter and video mode

libogc's post-retrace callback counts VIs and posts the game's retrace message every `retraceCount` frames
(`os_events.c`). The controller scan and the port's frame counter (`port_frame_now`, used by input scripts and logs)
run once per retrace event - not once per pump wake-up, which happens several times per frame.

`port/wii/main.c` picks the video mode in `port_pick_mode()`: 480p (`TVNtsc480Prog` / `TVEurgb60Hz480Prog`) when the
console is set to progressive scan and a component cable is present, otherwise `VIDEO_GetPreferredMode`. On a 16:9
console the VI output window is narrowed to 678 pixels so the 4:3 game keeps its proportions (pillarbox); the EFB and
XFB stay 640x480.

The game renders a 320x240 N64 framebuffer; the port renders the same scene into the 640x480 EFB at 2x, and Dolphin
can raise that further (internal resolution). Presentation (`port_present`) copies the EFB to an XFB on the RSP LWP;
the main LWP hands the XFB to the VI.

## Time, timers and frame pacing

- `osGetTime` / `osGetCount` emulate the N64 CPU counter (46.875 MHz) from the Wii timebase
  (`gettime() * 125 / 162`).
- `osGetTime` counts from the last `osSetTime`, exactly like libultra. The game calls `osSetTime(0)` on every retrace
  and later sleeps `13500 - OS_CYCLES_TO_USEC(osGetTime())` microseconds to pace its frame
  (`dm_game_graphic_onDoneSawp`). When `osSetTime` was a stub, that subtraction wrapped and the game could sleep for
  about 30 minutes - another "frozen picture" bug.
- `osSetTimer` / `osStopTimer` keep a small table; a timer LWP sleeps until the earliest expiry and posts the timer
  messages through the event ring. One-shot and periodic timers are supported (periodic ones do not drift).

## PI DMA and the embedded ROM

The Wii build embeds the uncompressed ROM image (built by the N64 build from the user's own ROM; never committed) in
the DOL as `port_rom`. `osPiStartDma` / `osEPiStartDma` are a `memcpy` from it into the destination buffer, followed by
the completion message - so asset loading works exactly as on the N64 (including gzip expansion where the game uses
it; with `UNCOMPRESSED_ROM` the boot code reads segments raw). `main_segment` itself is linked into the DOL, so the
boot path skips "DMA and decompress the main segment".

**Addresses.** N64 code passes pointers in several forms: KSEG0 virtual (`0x80xxxxxx`), physical (`0x00xxxxxx`, after
`K0_TO_PHYS`-style macros) and segmented (`0x0Sxxxxxx`, resolved through the RSP segment table). Every place where
the port turns a game address into a pointer (display-list walker, texture loads, audio commands, DMA destinations)
maps physical addresses to `0x80000000 | addr`, keeps `0x80`/`0x90` pointers, resolves segments, and **rejects** any
address inside the port's own code (`[__text_start, __etext)`, computed by the linker via `port/gen/wii_text.ld`).
Without that check a stray physical address once made the renderer read and then overwrite its own machine code.

N64 symbols that the game takes from asset segments (segmented addresses, `SEGMENT_ROM_START/END`) get their N64
values through `port/gen/n64_syms.ld`, generated from the N64 build by `port/tools/gen_syms.py`.

## Controllers

`port/wii/pad_wii.c` owns all libogc WPAD/PAD access; `port/ultra/os_cont.c` exposes the libultra controller API
(`osContStartQuery`, `osContGetReadData`, ...) from the cached result of one scan per retrace.

- Each of the four ports is probed separately: a Wiimote via `WPAD_Probe(i) == WPAD_ERR_NONE`, a GameCube pad via
  the bitmask returned by `PAD_ScanPads()` (bit i = port i - it is not a count). If both are on one port their
  buttons are ORed and the larger stick wins (`port_pad_merge`, host-tested).
- `WPAD_ScanPads()` must run every scan or Wiimote buttons never change.
- Wiimote held sideways: D-pad = stick/D-pad (rotated), 1 = B, 2 = A, + = Start, - = Z/L, B = R. Classic and
  GameCube controllers map 1:1; the Classic stick's raw unsigned value is centred and scaled to the N64's +-80.

## EEPROM saves

`port/ultra/os_eeprom.c` emulates the cartridge's 4 Kbit EEPROM (512 bytes) in RAM and writes the whole image to
`sd:/drmario/eeprom.bin` after every write. A missing file starts as an erased cartridge (0xFF), so the game
initialises a fresh save exactly as on a new cartridge.

## Graphics: RSP + RDP -> GX

On the N64 the game builds display lists that the RSP (running F3DEX2 or S2DEX microcode) transforms and the RDP
rasterises. The port's "RSP LWP" (`port/ultra/os_rsp.c`) receives each graphics task and `port/gfx/gfx.c` walks the
display list, keeping RSP/RDP state and drawing with GX:

- **Geometry** (`gfx_xform.c`, `gfx_clip.c`, `gfx_gx.c`): matrix stack, vertex loading and transformation, viewport,
  clipping, triangles and quads, fill rectangles, scissor (N64 1/4-pixel units at 2x).
- **TMEM emulation** (`gfx_tmem.c`): textures are loaded into a 4 KB TMEM model exactly like the RDP does,
  including two details textures depend on: LOADBLOCK swaps the 32-bit words of every odd line (the RDP's line
  counter advances by `dxt` per 64-bit word; `dxt = CALC_DXT`), and TLUTs are stored "quadricated" (entry i at
  `0x800 + i*8`). Tiles are decoded from TMEM by `gfx_texconv.c` (RGBA16/32, IA4/8/16, I4/8, CI4/8 with RGBA16 or IA16
  palettes) into GX textures, cached by address + format + size + content hash (`gfx_tex.c`).
- **Texture coordinates** (`gfx_tex.c`, `gfx_wrap.c`): a tile's sampled texel is `s - tile.uls`, `t - tile.ult` (10.2
  fixed point); wrap/mirror/clamp follow the RDP rules per axis (`cm` bit flags: 1 = mirror, 2 = clamp; repeat period
  `2^mask`; shift), with power-of-two GX textures for wrapping axes.
- **Combiner -> TEV** (`gfx_cc.c`): each N64 colour-combiner cycle `(A - B) * C + D` becomes three GX TEV stages
  (A*C, minus B*C, plus D, intermediate values unclamped), with primitive/environment/combined colours in TEV
  registers; 2-cycle mode uses six stages.
- **Render modes** (`gfx_rm.c`): blender, alpha compare, coverage-as-alpha and Z modes are mapped to GX blend, alpha
  compare and Z state. COPY and FILL cycle rectangles include their lower-right edge (as on the RDP); 1/2-cycle ones
  do not. In COPY cycle `dsdx` is in units of 4 texels.

### Off-screen render targets (`G_SETCIMG`)

The game sometimes points the RDP at a buffer in RAM instead of the display framebuffer. Gameplay does this once per
game: `dm_game_draw_snap_bg` draws the stage background, bottle and panels into `bg_snap_buf`, and every later frame
draws that buffer back as a texture (`CopyTexBlock16`). The port keeps the snapshot on the GPU: when the game switches
the colour image to such a buffer, the following draws go to the EFB and at the end of that task the EFB region is
copied (`GX_CopyTex`) into a GX texture that becomes an **alias** for the RAM buffer; later texture loads from inside
that buffer bind the alias (at the EFB's 2x resolution). Reading the copy back into RAM would not work in Dolphin's
default settings ("store EFB copies to texture only"). This path is correct for Classic; Story and Flash backgrounds
are still wrong (known issue, see the release notes).

### Whole-image textures (for HD texture packs)

Large pictures are drawn by the game's helpers in `src/main_segment/tex_func.c` as many small strips. Under
`TARGET_WII` each helper first emits a `G_NOOP` carrying a `PortImgHint` (`include/port_img_hint.h`: image pointer,
palette, size, format). The renderer then uploads the whole image once and draws every strip as a sub-rectangle of
it, so Dolphin sees one texture per picture - which is what makes clean, seam-free HD texture packs possible.

## Audio

- **Synthesis:** libmus and the libultra synthesizer run unchanged and produce an audio command list per frame for the
  RSP's audio microcode. `port/audio/naudio.c` interprets that command list in C (ADPCM decode, resampling, envelope
  mixer, filters, interleave) with a 4 KB DMEM model.
- **Synchronous execution:** on the N64 the audio scheduler pauses ("yields") the running graphics task, runs audio
  on the RSP, then resumes graphics. The port has no shared RSP to yield, so `nnScExecuteAudio` runs the audio
  command list directly on the audio thread (`port_audio_task_run`) - same results, no message juggling.
- **Output:** `osAiSetNextBuffer` / `osAiGetLength` / `osAiGetStatus` are backed by the Wii's audio-interface DMA
  (`AUDIO_InitDMA` / `AUDIO_StartDMA` and the DMA-done callback) with a 4-buffer ring - the closest match to the N64 AI,
  which is also a DMA FIFO of big-endian 16-bit stereo. (AESND was tried first; its voice failed to start in some
  builds, which made libmus compute a negative frame size and stall the music.)
- The game patches MIPS instructions into its own mixer to switch stereo/mono (`dm_audio_set_stereo`); that is skipped
  on the Wii, so the mono option currently has no effect.
