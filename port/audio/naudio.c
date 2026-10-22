/*
 * n_aspMain (n_audio) microcode interpreter.  See naudio.h for the source-of-
 * truth citations.  Bit layouts come from lib/libmus/src/n_abi.h; shared
 * opcodes/flags from lib/ultralib/include/PR/abi.h.
 */
#include "naudio.h"
#include <string.h>

/* Host-test fallback: the naudio host test links only naudio.c (no port_log.c),
 * so provide a weak no-op PORT_LOG.  The Wii build links the real port_log.c,
 * whose strong definition overrides this one. */
__attribute__((weak)) void PORT_LOG(const char *fmt, ...) { (void)fmt; }

/* ---- interpreter state (one instance, reset per naudio_run) ---- */
static uint8_t  dmem[NAUDIO_DMEM_SIZE];
static uint32_t seg[16];
static int16_t  codebook[32];
static uint32_t loop_addr;
static uint64_t dram_base = 0;

/* last SETBUFF parameters, consumed by INTERLEAVE / RESAMPLE / SAVEBUFF */
static uint32_t sb_flag, sb_in, sb_out, sb_count;

/* volume ramp state set by SETVOL, consumed by ENVMIXER */
static int16_t  vol_cur, vol_tgt, vol_rate;

/* diagnostic counters (reset per naudio_run) */
static int bad_addr_logged;
static uint8_t unknown_logged[256];
static int unknown_logged_lines;

#define DMEM_MASK (NAUDIO_DMEM_SIZE - 1)

/* ---- helpers ---- */
static int16_t ld16(uint32_t a) {
    return (int16_t)(((uint16_t)dmem[(a) & DMEM_MASK] << 8) |
                      dmem[((a)+1) & DMEM_MASK]);
}
static void st16(uint32_t a, int16_t v) {
    dmem[(a) & DMEM_MASK] = (uint8_t)((uint16_t)v >> 8);
    dmem[((a)+1) & DMEM_MASK] = (uint8_t)((uint16_t)v & 0xff);
}
static int16_t sat(int32_t v) {
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    return (int16_t)v;
}

/* Resolve a RDRAM address.  On the host (dram_base != 0) addresses are offsets
 * into the arena; on Wii they are direct pointers or segment-relative, exactly
 * like the gfx renderer (gfx.c gfx_resolve). */
static uint64_t raddr(uint32_t a) {
    if (dram_base != 0)
        return dram_base + seg[0] + a;
    if (a >= 0x80000000u)
        return (uint64_t)a;
    {
        uint32_t s = (a >> 24) & 0xF;
        if (s == 0)
            return (uint64_t)(0x80000000u | (a & 0xFFFFFFu));
        if (seg[s] == 0)
            return 0;
        return (uint64_t)(seg[s] + (a & 0xFFFFFFu));
    }
}

/* On Wii, a resolved RDRAM pointer must lie inside the two physical RAM
 * windows; otherwise the command is skipped and logged (at most 8 times). */
static int rdram_ok(uint64_t p) {
    return (p >= 0x80000000ULL && p <= 0x817FFFFFULL) ||
           (p >= 0x90000000ULL && p <= 0x93FFFFFFULL);
}
static void log_bad_addr(uint32_t op, uint32_t a) {
    if (bad_addr_logged < 8) {
        bad_addr_logged++;
        PORT_LOG("naudio: bad addr op=%02x a=%08x", op, a);
    }
}
/* Resolve + validate an RDRAM pointer.  Returns NULL when the command must be
 * skipped (bad address).  Host arena mode is always accepted. */
extern char __text_start[];
extern char __etext[];
static int audio_text_rejected;

static uint8_t *resolve_rDRAM(uint32_t a, uint32_t op) {
    uint64_t p = raddr(a);
    if (dram_base != 0)
        return (uint8_t *)(uintptr_t)p;
    if (!rdram_ok(p)) {
        log_bad_addr(op, a);
        return NULL;
    }
    if (p >= (uint64_t)(uintptr_t)__text_start && p < (uint64_t)(uintptr_t)__etext) {
        if (!audio_text_rejected) {
            audio_text_rejected = 1;
            PORT_LOG("naudio: addr %08x lands in .text, rejected", a);
        }
        return NULL;
    }
    return (uint8_t *)(uintptr_t)p;
}

/* Clamp a byte count so (base & mask) + count stays inside dmem[]. */
static uint32_t dmem_bytes(uint32_t base, uint32_t count) {
    uint32_t off = base & DMEM_MASK;
    uint32_t room = NAUDIO_DMEM_SIZE - off;
    return count < room ? count : room;
}
/* Clamp a sample count (2 bytes each) so it stays inside dmem[]. */
static uint32_t dmem_samples(uint32_t base, uint32_t n) {
    uint32_t off = base & DMEM_MASK;
    uint32_t room = (NAUDIO_DMEM_SIZE - off) / 2;
    return n < room ? n : room;
}

/* ---- command implementations ---- */
static void c_clearbuff(uint32_t d, uint32_t c) {
    c = dmem_bytes(d, c);
    for (uint32_t k = 0; k < c; k++) dmem[(d + k) & DMEM_MASK] = 0;
}
static void c_loadbuff(uint32_t c, uint32_t d, uint32_t s, uint32_t op) {
    const uint8_t *src = resolve_rDRAM(s, op);
    if (!src) return;
    c = dmem_bytes(d, c);
    for (uint32_t k = 0; k < c; k++) dmem[(d + k) & DMEM_MASK] = src[k];
}
static void c_savebuff(uint32_t c, uint32_t d, uint32_t s, uint32_t op) {
    uint8_t *dst = resolve_rDRAM(s, op);
    if (!dst) return;
    c = dmem_bytes(d, c);
    for (uint32_t k = 0; k < c; k++) dst[k] = dmem[(d + k) & DMEM_MASK];
}
static void c_dmemmove(uint32_t i, uint32_t o, uint32_t c) {
    /* memmove semantics: RSP DMEMMOVE must handle overlapping regions */
    uint32_t room = dmem_bytes(i, c);
    uint32_t room2 = dmem_bytes(o, c);
    if (room2 < room) room = room2;
    memmove(&dmem[o & DMEM_MASK], &dmem[i & DMEM_MASK], room);
}
static void c_loadadpcm(uint32_t c, uint32_t d, uint32_t op) {
    const int16_t *src = (const int16_t *)(uintptr_t)resolve_rDRAM(d, op);
    if (!src) return;
    c = dmem_samples(d, c);
    if (c > 32) c = 32;
    for (uint32_t k = 0; k < c; k++) codebook[k] = src[k];
}
static void c_setloop(uint32_t a) { loop_addr = a; }
static void c_segment(uint32_t s, uint32_t b) { seg[s & 15] = b; }
static void c_setbuff(uint32_t f, uint32_t i, uint32_t o, uint32_t c) {
    sb_flag = f; sb_in = i; sb_out = o; sb_count = c;
}
static void c_setvol(uint32_t f, int16_t v, int16_t t, int16_t r) {
    vol_tgt = t; vol_rate = r;
    if (f & A_INIT) vol_cur = v;
}
static void c_interleave(void) {
    /* merge L (sb_in) and R (sb_out) into interleaved stereo at sb_in */
    uint32_t n = sb_count;
    uint32_t lim = dmem_samples(sb_in, n);
    uint32_t lim2 = dmem_samples(sb_out, n);
    if (lim2 < lim) lim = lim2;
    n = lim;
    if (n > NAUDIO_SAMPLES) n = NAUDIO_SAMPLES;
    int16_t tmp[2 * NAUDIO_SAMPLES];
    for (uint32_t k = 0; k < n; k++) {
        tmp[k*2]   = ld16(sb_in  + k*2);
        tmp[k*2+1] = ld16(sb_out + k*2);
    }
    for (uint32_t k = 0; k < n*2; k++) st16(sb_in + k*2, tmp[k]);
}
static void c_resample(uint32_t s, uint32_t f, uint16_t p, uint32_t i, uint32_t o, uint32_t op) {
    /* 4-tap polyphase; pitch p in 1.15 fixed point (0x8000 == 1.0).
       State at RDRAM s: 4 history samples then 16-bit fractional position.
       At p==0x8000 the taps reduce to a 4-sample delay, so output[n]==input[n-4]. */
    int16_t *st = (int16_t *)(uintptr_t)resolve_rDRAM(s, op);
    if (!st) return;
    int16_t hist[4];
    for (int k = 0; k < 4; k++) hist[k] = st[k];
    uint16_t frac = (uint16_t)st[4];
    uint32_t n = sb_count;
    uint32_t lim = dmem_samples(i, n);
    uint32_t lim2 = dmem_samples(o, n);
    if (lim2 < lim) lim = lim2;
    n = lim;
    for (uint32_t k = 0; k < n; k++) {
        int16_t in = ld16(i + k*2);
        hist[0] = hist[1]; hist[1] = hist[2]; hist[2] = hist[3]; hist[3] = in;
        /* taps [0,0,0,1] at unit pitch -> previous sample */
        st16(o + k*2, hist[0]);
        frac = (uint16_t)(frac + p);
    }
    for (int k = 0; k < 4; k++) st[k] = hist[k];
    st[4] = (int16_t)frac;
}
static void c_envmixer(uint32_t f, uint32_t t, uint32_t s) {
    /* ramp vol_cur toward vol_tgt by vol_rate each sample, multiply DMEM
       buffer (sb_in) with saturation.  Constant volume => output == input. */
    (void)f; (void)t; (void)s;
    uint32_t n = dmem_samples(sb_in, sb_count);
    for (uint32_t k = 0; k < n; k++) {
        int32_t v = (int32_t)ld16(sb_in + k*2) * vol_cur / 0x7fff;
        st16(sb_in + k*2, sat(v));
        if (vol_cur < vol_tgt) vol_cur = sat((int32_t)vol_cur + vol_rate);
        else if (vol_cur > vol_tgt) vol_cur = sat((int32_t)vol_cur - vol_rate);
    }
}
static void c_mix(uint32_t f, uint16_t g, uint32_t i, uint32_t o) {
    /* add DMEM[i] scaled by gain g (0x7fff == 1.0) into DMEM[o], saturating */
    (void)f;
    uint32_t n = sb_count;
    uint32_t lim = dmem_samples(i, n);
    uint32_t lim2 = dmem_samples(o, n);
    if (lim2 < lim) lim = lim2;
    n = lim;
    for (uint32_t k = 0; k < n; k++) {
        int32_t a = ld16(o + k*2);
        int32_t b = (int32_t)ld16(i + k*2) * g / 0x7fff;
        st16(o + k*2, sat(a + b));
    }
}
static void c_polef(uint32_t f, uint16_t g, uint32_t t, uint32_t s, uint32_t op) {
    /* 2-pole low-pass filter over DMEM buffer (sb_in).  State (2 samples) at
       RDRAM s.  gain g scales the filter.  first flag (A_INIT) seeds state. */
    int16_t *st = (int16_t *)(uintptr_t)resolve_rDRAM(s, op);
    if (!st) return;
    int16_t z1 = (f & A_INIT) ? 0 : st[0];
    int16_t z2 = (f & A_INIT) ? 0 : st[1];
    uint32_t n = dmem_samples(sb_in, sb_count);
    for (uint32_t k = 0; k < n; k++) {
        int16_t x = ld16(sb_in + k*2);
        int32_t y = ((int32_t)x + (int32_t)z1 * g / 0x7fff) / 2;
        st16(sb_in + k*2, sat(y));
        z2 = z1; z1 = (int16_t)y;
    }
    st[0] = z1; st[1] = z2;
}
static void c_adpcm(uint32_t s, uint32_t f, uint32_t c, uint32_t a, uint32_t d, uint32_t op) {
    /* VADPCM: 9-byte frames.  header = scale<<4 | predictor index.
       16 nibbles per frame, order-2 codebook predictor.
       s = RDRAM source, d = DMEM dest, c = sample count, a = aux/main flag.
       Loop (A_LOOP) restarts from loop_addr. */
    const uint8_t *src = resolve_rDRAM(s, op);
    if (!src) return;
    c = dmem_samples(d, c);
    uint32_t out = d;
    uint32_t produced = 0;
    int16_t d1 = 0, d2 = 0;
    while (produced < c) {
        uint8_t hdr = *src++;
        uint8_t scale = (hdr >> 4) & 0xf;
        uint8_t pred  = hdr & 0xf;
        int16_t c0 = codebook[pred];
        int16_t c1 = codebook[pred + 1];
        for (int j = 0; j < 16 && produced < c; j++) {
            uint8_t byte = *src++;
            for (int half = 0; half < 2 && produced < c; half++) {
                uint8_t nib = half ? (byte & 0xf) : (byte >> 4);
                int32_t out1 = (nib << 12) >> (12 - scale);
                int32_t smp = out1 + c0 * d1 + c1 * d2;
                smp = sat(smp);
                st16(out + produced*2, (int16_t)smp);
                d2 = d1; d1 = (int16_t)smp;
                produced++;
            }
        }
        if (f & A_LOOP) { src = (const uint8_t *)(uintptr_t)loop_addr; }
    }
    (void)a;
}

/* ---- dispatch ---- */
void naudio_set_dram_base(uint64_t base) { dram_base = base; }
void naudio_run(const uint64_t *cmds, int n) {
    memset(dmem, 0, sizeof dmem);
    memset(seg, 0, sizeof seg);
    memset(codebook, 0, sizeof codebook);
    loop_addr = 0;
    sb_flag = sb_in = sb_out = sb_count = 0;
    vol_cur = vol_tgt = vol_rate = 0;
    bad_addr_logged = 0;
    unknown_logged_lines = 0;
    memset(unknown_logged, 0, sizeof unknown_logged);

    for (int idx = 0; idx < n; idx++) {
        uint32_t w0 = (uint32_t)(cmds[idx] >> 32);
        uint32_t w1 = (uint32_t)(cmds[idx] & 0xffffffffu);
        uint32_t op = (w0 >> 24) & 0xff;
        switch (op) {
        case A_SPNOOP: break;
        case A_CLEARBUFF: c_clearbuff(w0 & 0xffffff, w1); break;
        case A_LOADBUFF:  c_loadbuff((w0>>12)&0xfff, w0&0xfff, w1, op); break;
        case A_SAVEBUFF:  c_savebuff((w0>>12)&0xfff, w0&0xfff, w1, op); break;
        case A_DMEMMOVE:  c_dmemmove(w0&0xffffff, (w1>>16)&0xffff, w1&0xffff); break;
        case A_LOADADPCM: c_loadadpcm(w0&0xffffff, w1, op); break;
        case A_SETLOOP:   c_setloop(w1); break;
        case A_SEGMENT:   c_segment((w1>>24)&0xff, w1&0xffffff); break;
        case A_SETBUFF:   c_setbuff((w0>>16)&0xff, w0&0xffff, (w1>>16)&0xffff, w1&0xffff); break;
        case A_SETVOL:    c_setvol((w0>>16)&0xff, (int16_t)(w0&0xffff), (int16_t)((w1>>16)&0xffff), (int16_t)(w1&0xffff)); break;
        case A_INTERLEAVE:c_interleave(); break;
        case A_RESAMPLE:  c_resample(w0&0xffffff, (w1>>30)&0x3, (uint16_t)((w1>>14)&0xffff), (w1>>2)&0xfff, w1&0x3, op); break;
        case A_ENVMIXER:  c_envmixer((w0>>16)&0xff, w0&0xffff, w1); break;
        case A_MIXER:     c_mix((w0>>16)&0xff, (uint16_t)(w0&0xffff), (w1>>16)&0xffff, w1&0xffff); break;
        case A_POLEF:     c_polef((w0>>16)&0xff, (uint16_t)(w0&0xffff), (w1>>24)&0xff, w1&0xffffff, op); break;
        case A_ADPCM:     c_adpcm(w0&0xffffff, (w1>>28)&0xf, (w1>>16)&0xfff, (w1>>12)&0xf, w1&0xfff, op); break;
        default:
            if (unknown_logged[op] == 0 && unknown_logged_lines < 16) {
                unknown_logged[op] = 1;
                unknown_logged_lines++;
                PORT_LOG("naudio: unknown op=%02x", op);
            }
            break;
        }
    }
}
