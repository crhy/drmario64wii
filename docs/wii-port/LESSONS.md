# Lessons learned

Bugs and gotchas hit while porting, from `.strata/NOTES.md`. Each entry: symptom → cause → fix.

## Reliability

- **Frozen picture from dropped events (task 33).** Symptom: 1P Classic froze at frame 450 while presses logged at 644; SD log showed `ev/sec: post=1560 ok=1434 drop=125`. Cause: `port_deliver_event` (`port/ultra/os_sched.c`) dropped events when the target OSMesgQueue was full — on the N64 the receiver drains immediately so queues never fill. Fix: per-queue pending list, deliver on the `osRecvMesg` path in the same order; the `ev/sec` log must show `drop=0`.
- **Frame pacing / 30-minute sleeps.** `osSetTime` was a stub, so `osGetTime` counted from boot; `dm_game_graphic_onDoneSawp`'s `13500-OS_CYCLES_TO_USEC(osGetTime())` wrapped → ~30 min sleeps ("frozen picture"). Fix: `osGetTime` counts from the last `osSetTime`.
- **Event ring + timer races (12c).** Ring-full increments `g_ring_drops` and logs outside the IRQ-disabled region; timer scan is IRQ-guarded, due timers collected into a local array and posted after `port_irq_restore` (never post/usleep while IRQs disabled); periodic timers set `expiry += interval` with a `now+interval` clamp to kill drift; `osSetTimer`/`osStopTimer` wrap the `g_timers` find+write in `port_irq_disable`/`restore`.
- **Input script 4x-fast units.** The script clock and controller polling ran once per event-pump wake (~4 per VI: retrace+timer+SP+DP) instead of once per retrace. Fix: scan once per VI event. Frame numbers in scripts written before 2026-10-22 01:07 are in 4x-fast units.

## Audio

- **Audio freeze (task 15).** With music synthesis on, the whole Wii stopped at music frame ~106. Worked around with `-DPORT_NO_SYNTH`; root fixes were the `sched_current_slot_locked` idle bug, preempt-check from helper LWPs, `osSendMesg` from non-game LWPs, `port_present` writing the Wii XFB into the game's N64 framebuffer, and `PORT_LOG` fsync. AESND output was replaced with Wii AI DMA (`AUDIO_*`) in `os_stub.c` (15c4).
- **`os_stub.c` stubs of `makeScaleMatrix` and other calcsub helpers must be real C**, not no-ops — the renderer needs them.
- **naudio command count** must equal the array length (mixer block had 4 cmds but passed 3 → savebuff skipped → stale arena read); ADPCM frames encode nibbles high-first.

## Renderer

- **GX_SetArray stride gotcha:** `GX_VA_POS` with `GX_POS_XYZ`/`GX_S16` stride is `3*sizeof(s16)`, not 4 — using 4 produced a sheared white blob. CLR stride is `4*sizeof(u8)`.
- **`GX_SetAlphaCompare` arg order** is `(comp0, ref0, op, comp1, ref1)`; calling it `(0, GX_ALWAYS, AND, GX_ALWAYS, 0)` set `comp0=GX_NEVER`.
- **Transparent texels (19b):** missing `GX_SetZCompLoc(GX_FALSE)` when N64 alpha compare is active made alpha compare test interpolated vertex alpha instead of TEV/texture output alpha. Fix in `gfx_rm.c`: ZCompLoc false for alpha-compare draws, true otherwise.
- **TMEM LOADBLOCK odd-line swap:** LOADBLOCK leaves odd rows with adjacent 32-bit words swapped; undone on read by `addr^4` on odd rows. The odd-row rule uses the RDP counter (`+= dxt` per 64-bit word, odd = bit 11; `dxt = CALC_DXT = 2048/words`), not destination row parity (LOADTILE does use destination row parity).
- **TLUT entries are quadrupled:** entry i at `0x800 + i*8`, not `i*2`.
- **TLUT decode modes:** `tlut==2` (IA8) and `tlut==3` (IA16) share the same layout (I=high byte, A=low byte); decoding only `tlut==3` as IA made alpha-bearing TLUT entries flat grey. Add the tlut mode to the CI texture-cache key in `gfx_tex.c` so different TEXTLUT modes never collide.
- **EFB is 640 wide (open lead, parked task 30b):** some snap buffers are 328 wide, so the alias copy asks for `rtt_w*2 = 656`, wider than the EFB. This is the main suspect for the wrong Story/Flash backgrounds. A clamp to 640 was tried in 30b together with building the alias from RAM; that combination broke Classic and was reverted. Retry the clamp alone, gated on Classic staying identical.
- **Off-screen render targets (rtt):** the shipped code renders into the EFB and `GX_CopyTex`es it into a GPU alias texture (works with Dolphin's default "EFB copies to texture only"). Classic's background uses this path and is correct; do not replace it with a RAM-built alias without checking Classic frames.
- **EFB readback staleness:** reading `rtt_stage` right after `GX_CopyTex` without `GX_DrawDone` + `DCInvalidateRange` saw stale cache (zeros).
- **Dolphin `EFBToTextureEnable=True`** never writes EFB copies to RAM, so CPU readback of `GX_CopyTex` output is all zeros in default Dolphin.
- **Texture wrap/mirror/clamp (per-axis):** clamp bit → `GX_CLAMP` + POT padding; mask!=0 → `GX_REPEAT` (or `GX_MIRROR` if `cm&1`) with `gx_size=2^mask`; mask==0 → `GX_CLAMP`. Tile decoded RGBA8 to `2^mask` texels so `GX_REPEAT` wraps correctly.
- **GP desync:** `GX_DrawDone` in `port_present` never returned (first fill rect after a textured draw = title fade-out).
- **Game threads must call `GX_SetCurrentGXThread`** before GX work; GX FIFO moved to MEM2; VI configured before GX.

## Addressing

- **Physical `0x00xxxxxx` read as a display-list pointer** executed Wii PPC code as gfx commands and wrote into `.text` (canary fired "after gfx task 211"). Fix: every address→pointer site (`gfx_resolve`, `resolve_rDRAM`, `rsp_resolve_data`, `dma_resolve_dst`) maps `0x00-0x017FFFFF` → `0x80000000|addr`, keeps `0x80`/`0x90`, resolves `0x0Sxxxxxx` via the seg table, and rejects addresses inside `[__text_start,__etext)`.

## Build system

- **`player.c` #includes `player_commands.c`/`player_api.c`/`player_fifo.c` directly** — they must NOT be compiled as separate translation units (duplicate symbols); `GAME_SRCS` filters them out.
- **`-fcommon` vs `-fno-common` policy:** regular `.c` files use `-fcommon` (COMMON symbols, type C); `COMMON.o`/`scommon.o`/buffers use `-fno-common` (strong `.bss` defs, type B) so the defs merge with the `.c` COMMON refs without multiple-definition clash.
- **`Makefile.wii` CFLAGS needed `-Iinclude -I.` and `-DVERSION_US`** — `port/game/calcsub.c` includes `"calcsub.h"` (in `include/`) which pulls `version.h` (needs exactly one `VERSION_*`) and root-relative `"lib/ultralib/include/ultra64.h"`. `-Iinclude`/`-I.` go last so they cannot shadow libogc/PR headers.
- **`PORT_INPUT_SCRIPT` must be defined BEFORE the `$(ELF)` rule** so its prerequisites expand non-empty at parse time; the gate passes `-DPORT_INPUT_SCRIPT` only as a compiler macro (no make var), so the embed block is inert there.

## Logging

- **`printf` does NOT reach Dolphin's log** (newlib stdout isn't wired to OSReport); use `OSReport()` directly (`PORT_LOG`).
- **SD logging:** `WiiSDCardAllowWrites = True` in Dolphin.ini; `setvbuf` flush mode; `fflush`+`fsync` only when 32 lines pending, 1s of TBL ticks passed, or line starts "CRASH" (per-line fsync was too slow). `GetClock()` is not in this libogc's headers — use the inline `mftbu`/`mftb` TBL read.
- **Log mutex raced** when two threads logged first; `LWP_MutexInit` inside `port_log_init`.
- **`#ifdef PORT_LOG_VERBOSE` regression (12f2):** wrapping only the body of an `if` skipped the event-ring drain loop after the first 5 frames. A `#` comment inside a YAML folded (`>-`) block is NOT a comment — it swallows the command; put comments above the key.

## Controllers

- **`PAD_ScanPads()` returns a bitmask** (bit i = port i), not a count.
- **`WPAD_Probe(s32 chan, u32 *type)`** — enum is `WPAD_EXP_NONE/NUNCHUK/CLASSIC/...`, not `WPAD_PROBE_TYPE_*`; `WPAD_Expansion` returns void, test `exp.type==WPAD_EXP_CLASSIC` after; `expansion_t.classic.btns` is u32 with Classic bits in the HIGH 16 (`WPAD_CLASSIC_* = bit<<16`), so feed `btns>>16` to `port_map_classic`.
- **Missing `WPAD_ScanPads()`** meant Wiimote buttons never refreshed.
- **All libogc WPAD/PAD use lives in `port/wii/pad_wii.c`** with real headers; `os_cont.c` only calls `port_pad_scan` — hand-declared prototypes and wrong button enums in `os_cont.c` were deleted.

## Video

- **`VIDEO_GetPreferredMode(NULL)` on Dolphin Wii** returns a mode whose frame dump is 780x480, not 640x480 — don't assume 640x480 pixel coords when checking frames.
- **XFBs allocated twice** (main + `port_gx_setup`) leaked ~1.2MB MEM1 and ran `VIDEO_Configure` twice; allocate once in `main()` before `port_gx_setup`.
- **16:9 trick** pillarboxes the whole 4:3 image; it does not reveal more of the world. A true wide-background mode would widen only the background layer (wider ortho X range / wider bg texture sampling) while keeping the 4:3 gameplay field unscaled — that needs per-layer projection changes in `gfx_xform.c`, not a VI-mode tweak.

## Harness

- **`DR_DUMPEFB=1` desyncs the per-VI clock** from frame-numbered presses (dumping thousands of PNGs slows the emulator); never trust `DR_DUMPEFB` runs for game progression — cached cmp logs are the reliable signal.
- **X11/DRI3 harness blocker:** fresh headless runs die ~29-46 frames in with `MESA-EGL: DRI3 error` then X tears down; this is the harness (GL backend), not the code — reverting the edit crashes identically. Do not keep re-running the identical cmp command.
- **A hung Python process at ~100% CPU is an infinite loop in code you wrote**, not a slow machine; GTK child-walk loops must advance with `child = child.get_next_sibling()` on every path.

## Testing discipline

- Run just the new/changed test file alone under `timeout 30` before the full suite; run the full suite once at the end of a step.
- Host tests need shims for symbols the Wii build provides (e.g. `gfx_mem()` for the texconv test).
