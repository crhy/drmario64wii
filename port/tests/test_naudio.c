/* Host tests for the n_aspMain interpreter (port/audio/naudio.c). */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "naudio.h"

/* command-word builders mirroring n_abi.h / abi.h bit layouts */
static uint64_t mk(uint32_t w0, uint32_t w1) { return ((uint64_t)w0 << 32) | w1; }
static uint64_t c_clearbuff(uint32_t d, uint32_t c){ return mk((2u<<24)|(d&0xffffff), c); }
static uint64_t c_loadbuff(uint32_t cc,uint32_t d,uint32_t s){ return mk((4u<<24)|((cc&0xfff)<<12)|(d&0xfff), s); }
static uint64_t c_savebuff(uint32_t cc,uint32_t d,uint32_t s){ return mk((6u<<24)|((cc&0xfff)<<12)|(d&0xfff), s); }
static uint64_t c_dmemmove(uint32_t i,uint32_t o,uint32_t c){ return mk((10u<<24)|(i&0xffffff), ((o&0xffff)<<16)|(c&0xffff)); }
static uint64_t c_setbuff(uint32_t f,uint32_t i,uint32_t o,uint32_t c){ return mk((8u<<24)|((f&0xff)<<16)|(i&0xffff), ((o&0xffff)<<16)|(c&0xffff)); }
static uint64_t c_interleave(void){ return mk(13u<<24, 0); }
static uint64_t c_resample(uint32_t s,uint32_t f,uint16_t p,uint32_t i,uint32_t o){ return mk((5u<<24)|(s&0xffffff), ((f&3u)<<30)|(((uint32_t)p&0xffff)<<14)|((i&0xfff)<<2)|(o&3)); }
static uint64_t c_setvol(uint32_t f,int16_t v,int16_t t,int16_t r){ return mk((9u<<24)|((f&0xff)<<16)|((uint16_t)v), (((uint16_t)t)<<16)|((uint16_t)r)); }
static uint64_t c_envmixer(uint32_t f,uint32_t t,uint32_t s){ return mk((3u<<24)|((f&0xff)<<16)|(t&0xffff), s); }
static uint64_t c_mix(uint32_t f,uint16_t g,uint32_t i,uint32_t o){ return mk((12u<<24)|((f&0xff)<<16)|(g&0xffff), ((i&0xffff)<<16)|(o&0xffff)); }
static uint64_t c_adpcm(uint32_t s,uint32_t f,uint32_t c,uint32_t a,uint32_t d){ return mk((1u<<24)|(s&0xffffff), ((f&0xf)<<28)|((c&0xfff)<<16)|((a&0xf)<<12)|(d&0xfff)); }

/* RDRAM backing store: command addresses are offsets into this arena. */
static uint8_t arena[8192];
static int fails = 0;
#define CHECK(cond, name) do { if(!(cond)){ printf("FAIL %s\n", name); fails++; } } while(0)

/* write big-endian int16 samples into arena at byte offset */
static void put16(uint32_t off, const int16_t *v, int n) {
    for (int k = 0; k < n; k++) {
        arena[off + k*2]   = (uint8_t)((uint16_t)v[k] >> 8);
        arena[off + k*2+1] = (uint8_t)((uint16_t)v[k] & 0xff);
    }
}
/* read big-endian int16 samples from arena at byte offset */
static int16_t get16(uint32_t off, int k) {
    return (int16_t)(((uint16_t)arena[off + k*2] << 8) | arena[off + k*2+1]);
}

int main(void) {
    naudio_set_dram_base((uint64_t)(uintptr_t)arena);

    /* ---- CLEARBUFF ---- */
    {
        int16_t src[4] = {100,200,300,400};
        put16(0, src, 4);
        uint64_t cmds[] = {
            c_loadbuff(8, 0, 0),
            c_clearbuff(0, 8),
            c_savebuff(8, 0, 128),
        };
        naudio_run(cmds, 3);
        CHECK(get16(128,0)==0&&get16(128,1)==0&&get16(128,2)==0&&get16(128,3)==0, "clearbuff");
    }

    /* ---- LOAD -> SAVE round trip ---- */
    {
        int16_t src[4] = {1,2,3,4};
        put16(0, src, 4);
        uint64_t cmds[] = {
            c_loadbuff(8, 0, 0),
            c_savebuff(8, 0, 128),
        };
        naudio_run(cmds, 2);
        CHECK(get16(128,0)==1&&get16(128,1)==2&&get16(128,2)==3&&get16(128,3)==4, "load_save");
    }

    /* ---- DMEMMOVE ---- */
    {
        int16_t src[4] = {7,8,9,10};
        put16(0, src, 4);
        uint64_t cmds[] = {
            c_loadbuff(8, 0, 0),
            c_dmemmove(0, 4, 8),
            c_savebuff(8, 4, 128),
        };
        naudio_run(cmds, 3);
        CHECK(get16(128,0)==7&&get16(128,1)==8&&get16(128,2)==9&&get16(128,3)==10, "dmemmove");
    }

    /* ---- INTERLEAVE order ---- */
    {
        int16_t L[3] = {1,2,3}, R[3] = {10,20,30};
        put16(0, L, 3);
        put16(64, R, 3);
        uint64_t cmds[] = {
            c_loadbuff(6, 0, 0),
            c_loadbuff(6, 64, 64),
            c_setbuff(0, 0, 64, 3),
            c_interleave(),
            c_savebuff(12, 0, 128),
        };
        naudio_run(cmds, 5);
        CHECK(get16(128,0)==1&&get16(128,1)==10&&get16(128,2)==2&&
              get16(128,3)==20&&get16(128,4)==3&&get16(128,5)==30, "interleave");
    }

    /* ---- RESAMPLE at pitch 0x8000: 4-tap delay, output[n]==input[n-3] ---- */
    {
        int16_t in[8] = {100,200,300,400,500,600,700,800};
        put16(0, in, 8);
        /* state at arena offset 256: 4 history samples + frac, all zero */
        memset(&arena[256], 0, 10);
        uint64_t cmds[] = {
            c_loadbuff(16, 0, 0),
            c_setbuff(0, 0, 0, 8),
            c_resample(256, 0, 0x8000, 0, 0),
            c_savebuff(16, 0, 128),
        };
        naudio_run(cmds, 4);
        CHECK(get16(128,0)==0&&get16(128,1)==0&&get16(128,2)==0&&
              get16(128,3)==100&&get16(128,4)==200&&get16(128,5)==300&&
              get16(128,6)==400&&get16(128,7)==500, "resample_delay");
    }

    /* ---- MIXER saturation ---- */
    {
        int16_t buf[2] = {30000, -30000};
        put16(0, buf, 2);
        uint64_t cmds[] = {
            c_loadbuff(4, 0, 0),
            c_setbuff(0, 0, 0, 2),
            c_mix(0, 0x7fff, 0, 0),
            c_savebuff(4, 0, 128),
        };
        naudio_run(cmds, 4);
        CHECK(get16(128,0)==32767 && get16(128,1)==-32768, "mixer_saturation");
    }

    /* ---- ADPCM hand-encoded 16-sample frame, predictor 0, zero codebook ---- */
    {
        uint8_t frame[9] = {0x00, 0x01,0x23, 0x45,0x67, 0x89,0xab, 0xcd,0xef};
        memcpy(&arena[0], frame, 9);
        uint64_t cmds[] = {
            c_adpcm(0, 0, 16, 0, 0),
            c_savebuff(32, 0, 128),
        };
        naudio_run(cmds, 2);
        CHECK(get16(128,0)==0&&get16(128,1)==1&&get16(128,2)==2&&get16(128,3)==3&&
              get16(128,4)==4&&get16(128,5)==5&&get16(128,6)==6&&get16(128,7)==7, "adpcm_zero_codebook");
    }

    /* ---- ENVMIXER constant volume 0x7fff gives ~input ---- */
    {
        int16_t in[4] = {1000,-2000,3000,-4000};
        put16(0, in, 4);
        uint64_t cmds[] = {
            c_loadbuff(8, 0, 0),
            c_setvol(0x01, 0x7fff, 0x7fff, 0),
            c_setbuff(0, 0, 0, 4),
            c_envmixer(0, 0, 0),
            c_savebuff(8, 0, 128),
        };
        naudio_run(cmds, 5);
        CHECK(get16(128,0)==1000&&get16(128,1)==-2000&&get16(128,2)==3000&&get16(128,3)==-4000, "envmixer_const");
    }

    if (fails == 0) { printf("naudio tests OK\n"); return 0; }
    printf("%d failures\n", fails);
    return 1;
}
