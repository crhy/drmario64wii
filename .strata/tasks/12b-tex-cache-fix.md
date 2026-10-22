# Task 12b: texture cache correctness + speed (port/gfx/gfx_tex.c, gfx_bind path)
Bugs found in review, fix all of them:
1. WRONG TMEM ADDRESS: gfx_tex_bind hashes `tmem + t->tmem * 4u`, but TMEM byte address is tmem*8 (see gfx_tmem.c line
   ~145 and gfx_texconv.c line 33). So texture changes are missed (stale textures) and the hash can read past the end
   of the 4096-byte TMEM. Use base = t->tmem*8 and clamp the hashed length to 4096 - base.
2. PALETTES: for CI formats (fmt == 2, G_IM_FMT_CI) the cache key must include the TLUT bytes actually used
   (upper half of TMEM, 2048..4095, the 16 entries at palette*16 for 4-bit, all 256 for 8-bit) or a palette swap
   shows old colours.
3. USE-AFTER-FREE ON THE GPU: entries are free()d on collision/eviction while the GX FIFO may still reference them.
   Never free immediately: put the pointer on a pending-free list and free that list from a new function
   `void gfx_tex_frame_end(void)` that port_present() (port/wii/main.c) calls AFTER GX_DrawDone().
4. SPEED: bitwise CRC32 runs on up to 4 KB for EVERY textured triangle. Replace with a fast 32-bit-word hash
   (e.g. FNV-1a or murmur-style over u32 words) and add a "last bind" shortcut: a generation counter in gfx_tmem.c
   (`uint32_t gfx_tmem_gen(void)`, incremented by every LOADBLOCK/LOADTILE/LOADTLUT) — if gen, tile descriptor
   fields and otm_h filter bits equal the previous bind's, skip hashing AND skip GX_InitTexObj/GX_LoadTexObj entirely.
5. DCFlushRange only once, right after the texture is converted (not on every bind).
6. Replace the per-miss malloc for the RGBA8 temp with a static 32-byte-aligned buffer sized for the worst case
   (8192 texels * 4 bytes); skip with a log line if w*h is larger.
7. Make the cache 4-way set associative (64 sets x 4) with LRU within the set instead of direct-mapped 256.
Add counters (binds, skips, hits, misses) and PORT_LOG them every 600 frames from gfx_tex_frame_end.
GATE: make -f Makefile.wii >/dev/null 2>&1 && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t12b >/dev/null && .strata/imgmax.py .strata/scratch/t12b --min-colors 128 && grep -aqE "tex (binds|stats)" .strata/scratch/t12b/sd/drmario/log.txt
