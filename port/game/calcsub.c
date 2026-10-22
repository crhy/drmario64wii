#ifdef HOST_TEST
#define _LANGUAGE_C
#endif
#include <PR/ultratypes.h>
#include <PR/gbi.h>

#ifdef HOST_TEST
#undef _SIZE_T
#undef _SIZE_T_DEF
#include <stdint.h>
#define s32 int32_t
#define s16 int16_t
#define u32 uint32_t
#define u8  uint8_t
#define u64 uint64_t
#define vs32 volatile int32_t
#endif
#ifndef HOST_TEST
#include "calcsub.h"
#endif

#ifdef TARGET_WII
#include <math.h>
#endif

u8 D_800AB320[20] = {
    0x00, 0x03, 0x06, 0x08, 0x0A, 0x0D, 0x0F, 0x11,
    0x13, 0x15, 0x17, 0x19, 0x1B, 0x1C, 0x1E, 0x1F,
    0x20, 0x20, 0x20, 0x20,
};

s32 D_800AB334[34] = {
    0x00000000, 0x00648D18, 0x00C9393C, 0x012E239C,
    0x01936BB8, 0x01F93183, 0x025F958E, 0x02C6B932,
    0x032EBEBC, 0x0397C99D, 0x0401FE9D, 0x046D840F,
    0x04DA820D, 0x054922B8, 0x05B9927D, 0x062C0065,
    0x06A09E66, 0x0717A1C6, 0x07914383, 0x080DC0CD,
    0x088D5B8C, 0x09105AF7, 0x09970C44, 0x0A21C36D,
    0x0AB0DC15, 0x0B44BA8A, 0x0BDDCCF6, 0x0C7C8CBE,
    0x0D218015, 0x0DCD3BE0, 0x0E8065E3, 0x0F3BB757,
    0x10000000, 0xFFFFFFFF,
};

s32 D_800AB3BC[33] = {
    0x028BC48E, 0x028AFB8E, 0x02896A18, 0x02871113,
    0x0283F1FB, 0x02800EBC, 0x027B69B9, 0x027605D4,
    0x026FE65E, 0x02690F18, 0x02618440, 0x02594A7D,
    0x025066DF, 0x0246DEE6, 0x023CB868, 0x0231F9B4,
    0x0226A964, 0x021ACE77, 0x020E7038, 0x02019645,
    0x01F44893, 0x01E68F51, 0x01D872F9, 0x01C9FC39,
    0x01BB3404, 0x01AC2375, 0x019CD3D2, 0x018D4E95,
    0x017D9D49, 0x016DC99D, 0x015DDD57, 0x014DE244,
    0x013DE246,
};

s16 calc_sintable[1026] = {
    0x0000, 0x00C9, 0x0192, 0x025B, 0x0324, 0x03ED, 0x04B6, 0x057F,
    0x0648, 0x0711, 0x07D9, 0x08A2, 0x096B, 0x0A33, 0x0AFB, 0x0BC4,
    0x0C8C, 0x0D54, 0x0E1C, 0x0EE4, 0x0FAB, 0x1073, 0x113A, 0x1201,
    0x12C8, 0x138F, 0x1455, 0x151C, 0x15E2, 0x16A8, 0x176E, 0x1833,
    0x18F9, 0x19BE, 0x1A83, 0x1B47, 0x1C0C, 0x1CD0, 0x1D93, 0x1E57,
    0x1F1A, 0x1FDD, 0x209F, 0x2162, 0x2224, 0x22E5, 0x23A7, 0x2467,
    0x2528, 0x25E8, 0x26A8, 0x2768, 0x2827, 0x28E5, 0x29A4, 0x2A62,
    0x2B1F, 0x2BDC, 0x2C99, 0x2D55, 0x2E11, 0x2ECC, 0x2F87, 0x3042,
    0x30FC, 0x31B5, 0x326E, 0x3327, 0x33DF, 0x3497, 0x354E, 0x3604,
    0x36BA, 0x3770, 0x3825, 0x38D9, 0x398D, 0x3A40, 0x3AF3, 0x3BA5,
    0x3C57, 0x3D08, 0x3DB8, 0x3E68, 0x3F17, 0x3FC6, 0x4074, 0x4121,
    0x41CE, 0x427A, 0x4326, 0x43D1, 0x447B, 0x4524, 0x45CD, 0x4675,
    0x471D, 0x47C4, 0x486A, 0x490F, 0x49B4, 0x4A58, 0x4AFB, 0x4B9E,
    0x4C40, 0x4CE1, 0x4D81, 0x4E21, 0x4EC0, 0x4F5E, 0x4FFB, 0x5098,
    0x5134, 0x51CF, 0x5269, 0x5303, 0x539B, 0x5433, 0x54CA, 0x5560,
    0x55F6, 0x568A, 0x571E, 0x57B1, 0x5843, 0x58D4, 0x5964, 0x59F4,
    0x5A82, 0x5B10, 0x5B9D, 0x5C29, 0x5CB4, 0x5D3E, 0x5DC8, 0x5E50,
    0x5ED7, 0x5F5E, 0x5FE4, 0x6068, 0x60EC, 0x616F, 0x61F1, 0x6272,
    0x62F2, 0x6371, 0x63EF, 0x646C, 0x64E9, 0x6564, 0x65DE, 0x6657,
    0x66D0, 0x6747, 0x67BD, 0x6832, 0x68A7, 0x691A, 0x698C, 0x69FD,
    0x6A6E, 0x6ADD, 0x6B4B, 0x6BB8, 0x6C24, 0x6C8F, 0x6CF9, 0x6D62,
    0x6DCA, 0x6E31, 0x6E97, 0x6EFB, 0x6F5F, 0x6FC2, 0x7023, 0x7083,
    0x70E3, 0x7141, 0x719E, 0x71FA, 0x7255, 0x72AF, 0x7308, 0x735F,
    0x73B6, 0x740B, 0x7460, 0x74B3, 0x7505, 0x7556, 0x75A6, 0x75F4,
    0x7642, 0x768E, 0x76D9, 0x7723, 0x776C, 0x77B4, 0x77FB, 0x7840,
    0x7885, 0x78C8, 0x790A, 0x794A, 0x798A, 0x79C9, 0x7A06, 0x7A42,
    0x7A7D, 0x7AB7, 0x7AEF, 0x7B27, 0x7B5D, 0x7B92, 0x7BC6, 0x7BF9,
    0x7C2A, 0x7C5A, 0x7C89, 0x7CB7, 0x7CE4, 0x7D0F, 0x7D3A, 0x7D63,
    0x7D8A, 0x7DB1, 0x7DD6, 0x7DFB, 0x7E1E, 0x7E3F, 0x7E60, 0x7E7F,
    0x7E9D, 0x7EBA, 0x7ED6, 0x7EF0, 0x7F0A, 0x7F22, 0x7F38, 0x7F4E,
    0x7F62, 0x7F75, 0x7F87, 0x7F98, 0x7FA7, 0x7FB5, 0x7FC2, 0x7FCE,
    0x7FD9, 0x7FE2, 0x7FEA, 0x7FF1, 0x7FF6, 0x7FFA, 0x7FFE, 0x7FFF,
    0x7FFF, 0x7FFF, 0x7FFE, 0x7FFA, 0x7FF6, 0x7FF1, 0x7FEA, 0x7FE2,
    0x7FD9, 0x7FCE, 0x7FC2, 0x7FB5, 0x7FA7, 0x7F98, 0x7F87, 0x7F75,
    0x7F62, 0x7F4E, 0x7F38, 0x7F22, 0x7F0A, 0x7EF0, 0x7ED6, 0x7EBA,
    0x7E9D, 0x7E7F, 0x7E60, 0x7E3F, 0x7E1E, 0x7DFB, 0x7DD6, 0x7DB1,
    0x7D8A, 0x7D63, 0x7D3A, 0x7D0F, 0x7CE4, 0x7CB7, 0x7C89, 0x7C5A,
    0x7C2A, 0x7BF9, 0x7BC6, 0x7B92, 0x7B5D, 0x7B27, 0x7AEF, 0x7AB7,
    0x7A7D, 0x7A42, 0x7A06, 0x79C9, 0x798A, 0x794A, 0x790A, 0x78C8,
    0x7885, 0x7840, 0x77FB, 0x77B4, 0x776C, 0x7723, 0x76D9, 0x768E,
    0x7642, 0x75F4, 0x75A6, 0x7556, 0x7505, 0x74B3, 0x7460, 0x740B,
    0x73B6, 0x735F, 0x7308, 0x72AF, 0x7255, 0x71FA, 0x719E, 0x7141,
    0x70E3, 0x7083, 0x7023, 0x6FC2, 0x6F5F, 0x6EFB, 0x6E97, 0x6E31,
    0x6DCA, 0x6D62, 0x6CF9, 0x6C8F, 0x6C24, 0x6BB8, 0x6B4B, 0x6ADD,
    0x6A6E, 0x69FD, 0x698C, 0x691A, 0x68A7, 0x6832, 0x67BD, 0x6747,
    0x66D0, 0x6657, 0x65DE, 0x6564, 0x64E9, 0x646C, 0x63EF, 0x6371,
    0x62F2, 0x6272, 0x61F1, 0x616F, 0x60EC, 0x6068, 0x5FE4, 0x5F5E,
    0x5ED7, 0x5E50, 0x5DC8, 0x5D3E, 0x5CB4, 0x5C29, 0x5B9D, 0x5B10,
    0x5A82, 0x59F4, 0x5964, 0x58D4, 0x5843, 0x57B1, 0x571E, 0x568A,
    0x55F6, 0x5560, 0x54CA, 0x5433, 0x539B, 0x5303, 0x5269, 0x51CF,
    0x5134, 0x5098, 0x4FFB, 0x4F5E, 0x4EC0, 0x4E21, 0x4D81, 0x4CE1,
    0x4C40, 0x4B9E, 0x4AFB, 0x4A58, 0x49B4, 0x490F, 0x486A, 0x47C4,
    0x471D, 0x4675, 0x45CD, 0x4524, 0x447B, 0x43D1, 0x4326, 0x427A,
    0x41CE, 0x4121, 0x4074, 0x3FC6, 0x3F17, 0x3E68, 0x3DB8, 0x3D08,
    0x3C57, 0x3BA5, 0x3AF3, 0x3A40, 0x398D, 0x38D9, 0x3825, 0x3770,
    0x36BA, 0x3604, 0x354E, 0x3497, 0x33DF, 0x3327, 0x326E, 0x31B5,
    0x30FC, 0x3042, 0x2F87, 0x2ECC, 0x2E11, 0x2D55, 0x2C99, 0x2BDC,
    0x2B1F, 0x2A62, 0x29A4, 0x28E5, 0x2827, 0x2768, 0x26A8, 0x25E8,
    0x2528, 0x2467, 0x23A7, 0x22E5, 0x2224, 0x2162, 0x209F, 0x1FDD,
    0x1F1A, 0x1E57, 0x1D93, 0x1CD0, 0x1C0C, 0x1B47, 0x1A83, 0x19BE,
    0x18F9, 0x1833, 0x176E, 0x16A8, 0x15E2, 0x151C, 0x1455, 0x138F,
    0x12C8, 0x1201, 0x113A, 0x1073, 0x0FAB, 0x0EE4, 0x0E1C, 0x0D54,
    0x0C8C, 0x0BC4, 0x0AFB, 0x0A33, 0x096B, 0x08A2, 0x07D9, 0x0711,
    0x0648, 0x057F, 0x04B6, 0x03ED, 0x0324, 0x025B, 0x0192, 0x00C9,
    0x0000, -0x00C9, -0x0192, -0x025B, -0x0324, -0x03ED, -0x04B6, -0x057F,
    -0x0648, -0x0711, -0x07D9, -0x08A2, -0x096B, -0x0A33, -0x0AFB, -0x0BC4,
    -0x0C8C, -0x0D54, -0x0E1C, -0x0EE4, -0x0FAB, -0x1073, -0x113A, -0x1201,
    -0x12C8, -0x138F, -0x1455, -0x151C, -0x15E2, -0x16A8, -0x176E, -0x1833,
    -0x18F9, -0x19BE, -0x1A83, -0x1B47, -0x1C0C, -0x1CD0, -0x1D93, -0x1E57,
    -0x1F1A, -0x1FDD, -0x209F, -0x2162, -0x2224, -0x22E5, -0x23A7, -0x2467,
    -0x2528, -0x25E8, -0x26A8, -0x2768, -0x2827, -0x28E5, -0x29A4, -0x2A62,
    -0x2B1F, -0x2BDC, -0x2C99, -0x2D55, -0x2E11, -0x2ECC, -0x2F87, -0x3042,
    -0x30FC, -0x31B5, -0x326E, -0x3327, -0x33DF, -0x3497, -0x354E, -0x3604,
    -0x36BA, -0x3770, -0x3825, -0x38D9, -0x398D, -0x3A40, -0x3AF3, -0x3BA5,
    -0x3C57, -0x3D08, -0x3DB8, -0x3E68, -0x3F17, -0x3FC6, -0x4074, -0x4121,
    -0x41CE, -0x427A, -0x4326, -0x43D1, -0x447B, -0x4524, -0x45CD, -0x4675,
    -0x471D, -0x47C4, -0x486A, -0x490F, -0x49B4, -0x4A58, -0x4AFB, -0x4B9E,
    -0x4C40, -0x4CE1, -0x4D81, -0x4E21, -0x4EC0, -0x4F5E, -0x4FFB, -0x5098,
    -0x5134, -0x51CF, -0x5269, -0x5303, -0x539B, -0x5433, -0x54CA, -0x5560,
    -0x55F6, -0x568A, -0x571E, -0x57B1, -0x5843, -0x58D4, -0x5964, -0x59F4,
    -0x5A82, -0x5B10, -0x5B9D, -0x5C29, -0x5CB4, -0x5D3E, -0x5DC8, -0x5E50,
    -0x5ED7, -0x5F5E, -0x5FE4, -0x6068, -0x60EC, -0x616F, -0x61F1, -0x6272,
    -0x62F2, -0x6371, -0x63EF, -0x646C, -0x64E9, -0x6564, -0x65DE, -0x6657,
    -0x66D0, -0x6747, -0x67BD, -0x6832, -0x68A7, -0x691A, -0x698C, -0x69FD,
    -0x6A6E, -0x6ADD, -0x6B4B, -0x6BB8, -0x6C24, -0x6C8F, -0x6CF9, -0x6D62,
    -0x6DCA, -0x6E31, -0x6E97, -0x6EFB, -0x6F5F, -0x6FC2, -0x7023, -0x7083,
    -0x70E3, -0x7141, -0x719E, -0x71FA, -0x7255, -0x72AF, -0x7308, -0x735F,
    -0x73B6, -0x740B, -0x7460, -0x74B3, -0x7505, -0x7556, -0x75A6, -0x75F4,
    -0x7642, -0x768E, -0x76D9, -0x7723, -0x776C, -0x77B4, -0x77FB, -0x7840,
    -0x7885, -0x78C8, -0x790A, -0x794A, -0x798A, -0x79C9, -0x7A06, -0x7A42,
    -0x7A7D, -0x7AB7, -0x7AEF, -0x7B27, -0x7B5D, -0x7B92, -0x7BC6, -0x7BF9,
    -0x7C2A, -0x7C5A, -0x7C89, -0x7CB7, -0x7CE4, -0x7D0F, -0x7D3A, -0x7D63,
    -0x7D8A, -0x7DB1, -0x7DD6, -0x7DFB, -0x7E1E, -0x7E3F, -0x7E60, -0x7E7F,
    -0x7E9D, -0x7EBA, -0x7ED6, -0x7EF0, -0x7F0A, -0x7F22, -0x7F38, -0x7F4E,
    -0x7F62, -0x7F75, -0x7F87, -0x7F98, -0x7FA7, -0x7FB5, -0x7FC2, -0x7FCE,
    -0x7FD9, -0x7FE2, -0x7FEA, -0x7FF1, -0x7FF6, -0x7FFA, -0x7FFE, -0x7FFF,
    -0x8000, -0x7FFF, -0x7FFE, -0x7FFA, -0x7FF6, -0x7FF1, -0x7FEA, -0x7FE2,
    -0x7FD9, -0x7FCE, -0x7FC2, -0x7FB5, -0x7FA7, -0x7F98, -0x7F87, -0x7F75,
    -0x7F62, -0x7F4E, -0x7F38, -0x7F22, -0x7F0A, -0x7EF0, -0x7ED6, -0x7EBA,
    -0x7E9D, -0x7E7F, -0x7E60, -0x7E3F, -0x7E1E, -0x7DFB, -0x7DD6, -0x7DB1,
    -0x7D8A, -0x7D63, -0x7D3A, -0x7D0F, -0x7CE4, -0x7CB7, -0x7C89, -0x7C5A,
    -0x7C2A, -0x7BF9, -0x7BC6, -0x7B92, -0x7B5D, -0x7B27, -0x7AEF, -0x7AB7,
    -0x7A7D, -0x7A42, -0x7A06, -0x79C9, -0x798A, -0x794A, -0x790A, -0x78C8,
    -0x7885, -0x7840, -0x77FB, -0x77B4, -0x776C, -0x7723, -0x76D9, -0x768E,
    -0x7642, -0x75F4, -0x75A6, -0x7556, -0x7505, -0x74B3, -0x7460, -0x740B,
    -0x73B6, -0x735F, -0x7308, -0x72AF, -0x7255, -0x71FA, -0x719E, -0x7141,
    -0x70E3, -0x7083, -0x7023, -0x6FC2, -0x6F5F, -0x6EFB, -0x6E97, -0x6E31,
    -0x6DCA, -0x6D62, -0x6CF9, -0x6C8F, -0x6C24, -0x6BB8, -0x6B4B, -0x6ADD,
    -0x6A6E, -0x69FD, -0x698C, -0x691A, -0x68A7, -0x6832, -0x67BD, -0x6747,
    -0x66D0, -0x6657, -0x65DE, -0x6564, -0x64E9, -0x646C, -0x63EF, -0x6371,
    -0x62F2, -0x6272, -0x61F1, -0x616F, -0x60EC, -0x6068, -0x5FE4, -0x5F5E,
    -0x5ED7, -0x5E50, -0x5DC8, -0x5D3E, -0x5CB4, -0x5C29, -0x5B9D, -0x5B10,
    -0x5A82, -0x59F4, -0x5964, -0x58D4, -0x5843, -0x57B1, -0x571E, -0x568A,
    -0x55F6, -0x5560, -0x54CA, -0x5433, -0x539B, -0x5303, -0x5269, -0x51CF,
    -0x5134, -0x5098, -0x4FFB, -0x4F5E, -0x4EC0, -0x4E21, -0x4D81, -0x4CE1,
    -0x4C40, -0x4B9E, -0x4AFB, -0x4A58, -0x49B4, -0x490F, -0x486A, -0x47C4,
    -0x471D, -0x4675, -0x45CD, -0x4524, -0x447B, -0x43D1, -0x4326, -0x427A,
    -0x41CE, -0x4121, -0x4074, -0x3FC6, -0x3F17, -0x3E68, -0x3DB8, -0x3D08,
    -0x3C57, -0x3BA5, -0x3AF3, -0x3A40, -0x398D, -0x38D9, -0x3825, -0x3770,
    -0x36BA, -0x3604, -0x354E, -0x3497, -0x33DF, -0x3327, -0x326E, -0x31B5,
    -0x30FC, -0x3042, -0x2F87, -0x2ECC, -0x2E11, -0x2D55, -0x2C99, -0x2BDC,
    -0x2B1F, -0x2A62, -0x29A4, -0x28E5, -0x2827, -0x2768, -0x26A8, -0x25E8,
    -0x2528, -0x2467, -0x23A7, -0x22E5, -0x2224, -0x2162, -0x209F, -0x1FDD,
    -0x1F1A, -0x1E57, -0x1D93, -0x1CD0, -0x1C0C, -0x1B47, -0x1A83, -0x19BE,
    -0x18F9, -0x1833, -0x176E, -0x16A8, -0x15E2, -0x151C, -0x1455, -0x138F,
    -0x12C8, -0x1201, -0x113A, -0x1073, -0x0FAB, -0x0EE4, -0x0E1C, -0x0D54,
    -0x0C8C, -0x0BC4, -0x0AFB, -0x0A33, -0x096B, -0x08A2, -0x07D9, -0x0711,
    -0x0648, -0x057F, -0x04B6, -0x03ED, -0x0324, -0x025B, -0x0192, -0x00C9,
    0x0000, 0x0000,
};


void waitEQ4(vs32 *var, s32 data) {
    while (*var != data) {
    }
}

s32 sqrtS(s32 value) {
    return (s32)sqrtf((float)value);
}

s32 sqrtL(s32 arg0, s32 arg1) {
    u32 v0 = 0, v1 = 0;
    s32 t8, t9, i;
    t8 = (arg1 < 0);
    t9 = (arg0 < 0);
    arg1 = arg1 << 1;
    arg0 = (arg0 << 1) | t8;
    v1 = (v1 << 1) | (u32)t9;
    for (i = 0; i < 32; i++) {
        t8 = (arg1 < 0);
        arg1 = arg1 << 1;
        t9 = (arg0 < 0);
        arg0 = (arg0 << 1) | t8;
        v1 = (v1 << 1) | (u32)t9;
        v0 = (v0 << 1) + 1;
        if (v1 >= v0) {
            v1 -= v0;
            v0 += 2;
        }
        t8 = (arg1 < 0);
        arg1 = arg1 << 1;
        t9 = (arg0 < 0);
        arg0 = (arg0 << 1) | t8;
        v1 = (v1 << 1) | (u32)t9;
        v0 = v0 - 1;
    }
    return (s32)(v0 >> 1);
}

s32 get_angle(s32 x, s32 y) {
    s32 angle;
    u32 flags;
    u32 a0;
    u32 idx;
    s32 t9;

    if (x == 0) {
        if (y == 0) {
            return 0;
        }
        if (y < 0) {
            return (s32)(s16)0xC000;
        }
        return (s32)(s16)0x4000;
    }

    flags = 0;
    if (x < 0) {
        x = -x;
        flags = 0x100 | 0x10;
    }

    if (y == 0) {
        if (flags == 0) {
            return 0;
        }
        return (s32)(s16)0x8000;
    }

    if (y < 0) {
        y = -y;
        flags ^= 0x10;
    }

    if (x < y) {
        s32 tmp = x;
        x = y;
        y = tmp;
        flags ^= 0x1;
    }

    a0 = ((0x40000000U / (u32)x) * (u32)y) >> 2;

    idx = D_800AB320[(a0 >> 24) & 0xFF];
    do {
        t9 = D_800AB334[idx];
        idx++;
    } while ((s32)(a0 - (u32)t9) >= 0);

    idx -= 2;
    t9 = D_800AB334[idx];
    angle = idx << 8;
    if (a0 != (u32)t9) {
        u64 hi = ((u64)(u32)(a0 - (u32)t9) * (u64)(u32)D_800AB3BC[idx]) >> 32;
        angle |= (s32)((hi >> 8) & 0xFF);
    }

    if (flags & 0x1) {
        angle = 0x4000 - angle;
    }
    if (flags & 0x10) {
        angle = -angle;
    }
    if (flags & 0x100) {
        angle -= 0x8000;
    }
    return (s32)(s16)angle;
}

s16 sinL(s16 angle) {
    u32 a = (u32)angle;
    u32 upper = (a >> 5) & 0x7FE;
    u32 low = a & 0x3F;
    s32 ret = calc_sintable[upper >> 1];
    if (low != 0) {
        s32 next = calc_sintable[(upper >> 1) + 1];
        ret += ((next - ret) * (s32)low) >> 6;
    }
    return (s16)ret;
}

s16 cosL(s16 angle) {
    u32 a = (u32)(angle + 0x4000);
    u32 upper = (a >> 5) & 0x7FE;
    u32 low = a & 0x3F;
    s32 ret = calc_sintable[upper >> 1];
    if (low != 0) {
        s32 next = calc_sintable[(upper >> 1) + 1];
        ret += ((next - ret) * (s32)low) >> 6;
    }
    return (s16)ret;
}

/*
 * void lc2wc(Mtx *arg0, s32 arg1, s32 arg2, s32 arg3, s32 *arg4, s32 *arg5, s32 *arg6);
 *
 * Local -> world. For each output axis i, the assembly (calcsub.s:675) computes
 *   value = (s64)m[0][i]*arg1 + (s64)m[1][i]*arg2 + (s64)m[2][i]*arg3   (64-bit sum)
 * then shifted = (lo >> 15) | (hi << 17) == (s32)(value >> 15), and stores
 *   *out[i] = m[3][i] + shifted.
 * out[] = {arg4, arg5, arg6}.
 */
void lc2wc(Mtx *arg0, s32 arg1, s32 arg2, s32 arg3, s32 *arg4, s32 *arg5, s32 *arg6) {
    s32 *arr[] = {arg4, arg5, arg6};
    s32 i;

    for (i = 0; i < 3; i++) {
        s64 value = (s64)arg0->m[0][i] * (s64)arg1
                  + (s64)arg0->m[1][i] * (s64)arg2
                  + (s64)arg0->m[2][i] * (s64)arg3;
        s32 shifted = (s32)(value >> 15);
        *arr[i] = (s32)(arg0->m[3][i] + shifted);
    }
}

/*
 * void wc2lc(Mtx *arg0, s32 arg1, s32 arg2, s32 arg3, s32 *arg4, s32 *arg5, s32 *arg6);
 *
 * World -> local. The assembly (calcsub.s:754) first subtracts the translation row
 *   arg1 -= m[3][0]; arg2 -= m[3][1]; arg3 -= m[3][2];
 * then for each row i sums the three products into a 64-bit value, takes
 *   shifted = (lo >> 15) | (hi << 17) == (s32)(value >> 15), and stores *out[i] = shifted.
 */
void wc2lc(Mtx *arg0, s32 arg1, s32 arg2, s32 arg3, s32 *arg4, s32 *arg5, s32 *arg6) {
    s32 *arr[] = {arg4, arg5, arg6};
    s32 i;

    arg1 -= (s32)arg0->m[3][0];
    arg2 -= (s32)arg0->m[3][1];
    arg3 -= (s32)arg0->m[3][2];

    for (i = 0; i < 3; i++) {
        s64 value = (s64)arg0->m[i][0] * (s64)arg1
                  + (s64)arg0->m[i][1] * (s64)arg2
                  + (s64)arg0->m[i][2] * (s64)arg3;
        s32 shifted = (s32)(value >> 15);
        *arr[i] = shifted;
    }
}

/*
 * void matrixMulL(const Mtx *m, const Mtx *n, Mtx *r);
 *
 * r = m * n (calcsub.s:854). The last column of r is forced to [0,0,0,0x8000].
 * For each row i and column j (j<3): value = sum_k m[i][k]*n[k][j] (64-bit),
 *   shifted = (lo >> 15) | (hi << 17) == (s32)(value >> 15), r[i][j] = shifted.
 * For the translation row: value = sum_k m[3][k]*n[k][i], then r[3][i] = n[3][i] + shifted.
 */
void matrixMulL(const Mtx *m, const Mtx *n, Mtx *r) {
    s32 i;
    s32 j;
    s32 k;

    r->m[0][3] = 0;
    r->m[1][3] = 0;
    r->m[2][3] = 0;
    r->m[3][3] = 0x8000;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            s64 value = 0;
            for (k = 0; k < 3; k++) {
                value += (s64)m->m[i][k] * (s64)n->m[k][j];
            }
            s32 shifted = (s32)(value >> 15);
            r->m[i][j] = shifted;
        }

        s64 value = 0;
        for (k = 0; k < 3; k++) {
            value += (s64)m->m[3][k] * (s64)n->m[k][i];
        }
        s32 shifted = (s32)(value >> 15);
        r->m[3][i] = (s32)(n->m[3][i] + shifted);
    }
}

/*
 * void makeTransrateMatrix(Mtx *m, s32 xofs, s32 yofs, s32 zofs);
 *
 * calcsub.s:1046: identity with 0x8000 on the diagonal and the offsets in the last row.
 */
void makeTransrateMatrix(Mtx *m, s32 xofs, s32 yofs, s32 zofs) {
    m->m[0][0] = 0x8000;
    m->m[0][1] = 0;
    m->m[0][2] = 0;
    m->m[0][3] = 0;
    m->m[1][0] = 0;
    m->m[1][1] = 0x8000;
    m->m[1][2] = 0;
    m->m[1][3] = 0;
    m->m[2][0] = 0;
    m->m[2][1] = 0;
    m->m[2][2] = 0x8000;
    m->m[2][3] = 0;
    m->m[3][0] = xofs;
    m->m[3][1] = yofs;
    m->m[3][2] = zofs;
    m->m[3][3] = 0x8000;
}

/*
 * void makeScaleMatrix(Mtx *m, s32 scale);
 *
 * calcsub.s:1099: diagonal = scale (except m[3][3] = 0x8000), everything else 0.
 */
void makeScaleMatrix(Mtx *m, s32 scale) {
    m->m[0][0] = scale;
    m->m[0][1] = 0;
    m->m[0][2] = 0;
    m->m[0][3] = 0;
    m->m[1][0] = 0;
    m->m[1][1] = scale;
    m->m[1][2] = 0;
    m->m[1][3] = 0;
    m->m[2][0] = 0;
    m->m[2][1] = 0;
    m->m[2][2] = scale;
    m->m[2][3] = 0;
    m->m[3][0] = 0;
    m->m[3][1] = 0;
    m->m[3][2] = 0;
    m->m[3][3] = 0x8000;
}

/*
 * void makeMatrix(Mtx *m, s16 xrot, s16 yrot, s16 zrot, s32 xofs, s32 yofs, s32 zofs);
 *
 * calcsub.s:1170. Combined XYZ rotation matrix (row-major, 16.15 fixed point).
 * Products of two s16 trig values fit in s32; the addu/subu of two such products
 * can wrap in s32, so the sum is computed in s64 and truncated to s32 before the
 * arithmetic >>15, matching the assembly's low-32-bit semantics.
 */
void makeMatrix(Mtx *m, s16 xrot, s16 yrot, s16 zrot, s32 xofs, s32 yofs, s32 zofs) {
    s32 sin_y = sinL(yrot);
    s32 cos_y = cosL(yrot);
    s32 sin_z = sinL(zrot);
    s32 cos_z = cosL(zrot);
    s32 sin_x = sinL(xrot);
    s32 cos_x = cosL(xrot);
    s32 a = (sin_y * sin_x) >> 15;
    s32 b = (cos_y * sin_x) >> 15;

    m->m[0][3] = 0;
    m->m[1][3] = 0;
    m->m[2][3] = 0;
    m->m[3][3] = 0x8000;
    m->m[3][0] = xofs;
    m->m[3][1] = yofs;
    m->m[3][2] = zofs;

    m->m[2][1] = -sin_x;
    m->m[0][1] = (sin_z * cos_x) >> 15;
    m->m[1][1] = (cos_z * cos_x) >> 15;
    m->m[2][0] = (sin_y * cos_x) >> 15;
    m->m[2][2] = (cos_y * cos_x) >> 15;
    m->m[0][0] = (s32)((s64)(sin_z * a) + (s64)(cos_z * cos_y)) >> 15;
    m->m[1][0] = (s32)((s64)(cos_z * a) - (s64)(sin_z * cos_y)) >> 15;
    m->m[0][2] = (s32)((s64)(sin_z * b) - (s64)(cos_z * sin_y)) >> 15;
    m->m[1][2] = (s32)((s64)(cos_z * b) + (s64)(sin_z * sin_y)) >> 15;
}

/*
 * void makeXrotMatrix(Mtx *m, s16 xrot, s32 xofs, s32 yofs, s32 zofs);
 *
 * calcsub.s:1306; matches the doc comment in the assembly.
 */
void makeXrotMatrix(Mtx *m, s16 xrot, s32 xofs, s32 yofs, s32 zofs) {
    s32 sin_x = sinL(xrot);
    s32 cos_x = cosL(xrot);

    m->m[0][3] = 0;
    m->m[1][3] = 0;
    m->m[2][3] = 0;
    m->m[3][0] = xofs;
    m->m[3][1] = yofs;
    m->m[3][2] = zofs;
    m->m[3][3] = 0x8000;

    m->m[0][0] = 0x8000;
    m->m[0][1] = 0;
    m->m[0][2] = 0;
    m->m[1][0] = 0;
    m->m[1][1] = cos_x;
    m->m[1][2] = sin_x;
    m->m[2][0] = 0;
    m->m[2][1] = -sin_x;
    m->m[2][2] = cos_x;
}

/*
 * void makeYrotMatrix(Mtx *m, s16 yrot, s32 xofs, s32 yofs, s32 zofs);
 *
 * calcsub.s:1384; matches the doc comment in the assembly.
 */
void makeYrotMatrix(Mtx *m, s16 yrot, s32 xofs, s32 yofs, s32 zofs) {
    s32 sin_y = sinL(yrot);
    s32 cos_y = cosL(yrot);

    m->m[0][3] = 0;
    m->m[1][3] = 0;
    m->m[2][3] = 0;
    m->m[3][0] = xofs;
    m->m[3][1] = yofs;
    m->m[3][2] = zofs;
    m->m[3][3] = 0x8000;

    m->m[0][0] = cos_y;
    m->m[0][1] = 0;
    m->m[0][2] = -sin_y;
    m->m[1][0] = 0;
    m->m[1][1] = 0x8000;
    m->m[1][2] = 0;
    m->m[2][0] = sin_y;
    m->m[2][1] = 0;
    m->m[2][2] = cos_y;
}

/*
 * void makeZrotMatrix(Mtx *m, s16 zrot, s32 xofs, s32 yofs, s32 zofs);
 *
 * calcsub.s:1462; matches the doc comment in the assembly.
 */
void makeZrotMatrix(Mtx *m, s16 zrot, s32 xofs, s32 yofs, s32 zofs) {
    s32 sin_z = sinL(zrot);
    s32 cos_z = cosL(zrot);

    m->m[0][3] = 0;
    m->m[1][3] = 0;
    m->m[2][3] = 0;
    m->m[3][0] = xofs;
    m->m[3][1] = yofs;
    m->m[3][2] = zofs;
    m->m[3][3] = 0x8000;

    m->m[0][0] = cos_z;
    m->m[0][1] = sin_z;
    m->m[0][2] = 0;
    m->m[1][0] = -sin_z;
    m->m[1][1] = cos_z;
    m->m[1][2] = 0;
    m->m[2][0] = 0;
    m->m[2][1] = 0;
    m->m[2][2] = 0x8000;
}

/*
 * void makeXZMatrix(Mtx *m, s16 xrot, s16 zrot);
 *
 * calcsub.s:1545; matches the doc comment in the assembly.
 */
void makeXZMatrix(Mtx *m, s16 xrot, s16 zrot) {
    s32 sin_z = sinL(zrot);
    s32 cos_z = cosL(zrot);
    s32 sin_x = sinL(xrot);
    s32 cos_x = cosL(xrot);

    m->m[0][3] = 0;
    m->m[1][3] = 0;
    m->m[2][3] = 0;
    m->m[3][0] = 0;
    m->m[3][1] = 0;
    m->m[3][2] = 0;
    m->m[3][3] = 0x8000;

    m->m[0][0] = cos_z;
    m->m[0][1] = sin_z;
    m->m[0][2] = 0;
    m->m[1][0] = (-sin_z * cos_x) >> 15;
    m->m[1][1] = (cos_z * cos_x) >> 15;
    m->m[1][2] = sin_x;
    m->m[2][0] = (sin_z * sin_x) >> 15;
    m->m[2][1] = (-sin_x * cos_z) >> 15;
    m->m[2][2] = cos_x;
}

/*
 * void matrixConv(const Mtx *m, Mtx *mtx, s32 shift);
 *
 * calcsub.s:1648. Reads 8 words from m, writes 16 words to mtx: for each
 * source word pair (w0, w1) shifted (srav by -shift-1 if shift<0, sllv by
 * shift+1 if shift>0, sll 1 if shift==0), the halves are interleaved:
 * mtx word k = (hi16(t2) << 16) | (t3 >>u 16), word k+8 = (t2 << 16) | lo16(t3).
 */
void matrixConv(const Mtx *m, Mtx *mtx, s32 shift) {
    const s32 *a0 = (const s32 *)&m->m[0][0];
    s32 *a1 = (s32 *)&mtx->m[0][0];
    s32 temp_a2;
    s32 temp_t2;
    s32 temp_t3;
    s32 var_v0;

    if (shift != 0) {
        var_v0 = 6;
        if (shift <= 0) {
            temp_a2 = -(shift + 1);
            for (; var_v0 > 0; var_v0--) {
                temp_t2 = (*a0++) >> temp_a2;
                temp_t3 = (*a0++) >> temp_a2;
                a1[0] = (temp_t2 & 0xFFFF0000) | (s32)(((u32)temp_t3) >> 0x10);
                a1[8] = (temp_t2 << 0x10) | (temp_t3 & 0xFFFF);
                a1++;
            }
        } else {
            temp_a2 = shift + 1;
            for (; var_v0 > 0; var_v0--) {
                temp_t2 = (*a0++) << temp_a2;
                temp_t3 = (*a0++) << temp_a2;
                a1[0] = (temp_t2 & 0xFFFF0000) | (s32)(((u32)temp_t3) >> 0x10);
                a1[8] = (temp_t2 << 0x10) | (temp_t3 & 0xFFFF);
                a1++;
            }
        }
        var_v0 = 2;
    } else {
        var_v0 = 8;
    }

    for (; var_v0 > 0; var_v0--) {
        temp_t2 = (*a0++) << 1;
        temp_t3 = (*a0++) << 1;
        a1[0] = (temp_t2 & 0xFFFF0000) | (s32)(((u32)temp_t3) >> 0x10);
        a1[8] = (temp_t2 << 0x10) | (temp_t3 & 0xFFFF);
        a1++;
    }
}

/*
 * void matrixCopyL(Mtx *dst, const Mtx *src);
 *
 * calcsub.s:1740: copies 64 bytes (4 x 16) from src to dst.
 */
void matrixCopyL(Mtx *dst, const Mtx *src) {
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            dst->m[i][j] = src->m[i][j];
        }
    }
}

/*
 * void rotpointL(s16 angle, s32 center_x, s32 center_y, s32 *x, s32 *y);
 *
 * calcsub.s:1793. cos/sin are s16 sign-extended to s32; mult is signed 32x32->64.
 * The 64-bit result is recombined as (lo >>u 15) | (hi << 17) == value >>s 15.
 *   *y = center_y + ((delta_x * sin + delta_y * cos) >> 15)
 *   *x = center_x + ((delta_x * cos - delta_y * sin) >> 15)
 */
void rotpointL(s16 angle, s32 center_x, s32 center_y, s32 *x, s32 *y) {
    s64 cos = (s64)(s32)cosL(angle);
    s64 sin = (s64)(s32)sinL(angle);
    s64 delta_y = (s64)(*y - center_y);
    s64 delta_x = (s64)(*x - center_x);
    s64 value;
    s32 hi;
    s32 lo;
    s32 shifted;

    value = delta_x * sin + delta_y * cos;
    hi = (s32)(((u64)value) >> 32);
    lo = (s32)(((u64)value) & 0xFFFFFFFF);
    shifted = (lo >> 15) | (hi << 17);

    *y = center_y + shifted;

    value = delta_x * cos - delta_y * sin;
    hi = (s32)(((u64)value) >> 32);
    lo = (s32)(((u64)value) & 0xFFFFFFFF);
    shifted = (lo >> 15) | (hi << 17);

    *x = center_x + shifted;
}

/*
 * s32 defangleL(s32 a1, s32 a2);
 *
 * calcsub.s:1878: shortest signed difference between two 16-bit angles.
 */
s32 defangleL(s32 a1, s32 a2) {
    s32 angle1 = a1 & 0xFFFF;
    s32 angle2 = a2 & 0xFFFF;
    s32 angle01;
    s32 angle02;
    s32 abs2;
    s32 abs02;

    if (angle1 < angle2) {
        angle01 = angle1 + 0x10000;
        angle02 = angle2;
    } else {
        angle01 = angle1;
        angle02 = angle2 + 0x10000;
    }

    angle2 -= angle1;
    angle02 -= angle01;
    abs2 = angle2 < 0 ? -angle2 : angle2;
    abs02 = angle02 < 0 ? -angle02 : angle02;
    if (abs2 > abs02) {
        return angle02;
    }
    return angle2;
}

/*
 * s32 distanceS(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1);
 *
 * calcsub.s:1925: integer diffs, integer squares (32-bit wrap), one integer
 * sum, converted to float once, sqrtf, truncated toward zero.
 */
s32 distanceS(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1) {
    s32 x_diff = x1 - x0;
    s32 y_diff = y1 - y0;
    s32 z_diff = z1 - z0;
    s32 sum = x_diff * x_diff + y_diff * y_diff + z_diff * z_diff;

    return (s32)sqrtf((f32)sum);
}

/*
 * s32 distanceL(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1);
 *
 * calcsub.s:1971: diffs converted to float, float squares,
 * sq = z2 + (x2 + y2), sqrtf, truncated toward zero.
 */
s32 distanceL(s32 x0, s32 y0, s32 z0, s32 x1, s32 y1, s32 z1) {
    f32 x_diff = (f32)(x1 - x0);
    f32 y_diff = (f32)(y1 - y0);
    f32 z_diff = (f32)(z1 - z0);
    f32 sq = z_diff * z_diff + (x_diff * x_diff + y_diff * y_diff);

    return (s32)sqrtf(sq);
}

/*
 * s32 sqrt_a2b2(s32 a, s32 b);
 *
 * calcsub.s:2009: sq = a2 + b2 in float, sqrtf, truncated toward zero.
 */
s32 sqrt_a2b2(s32 a, s32 b) {
    f32 fa = (f32)a;
    f32 fb = (f32)b;
    f32 sq = fa * fa + fb * fb;

    return (s32)sqrtf(sq);
}

/*
 * s32 sqrt_abc(s32 arg0, s32 arg1, s32 arg2);
 *
 * calcsub.s:2034: sq = (a2 + b2) + c2 in float, sqrtf, truncated toward zero.
 */
s32 sqrt_abc(s32 a, s32 b, s32 c) {
    f32 fa = (f32)a;
    f32 fb = (f32)b;
    f32 fc = (f32)c;
    f32 sq = (fa * fa + fb * fb) + fc * fc;

    return (s32)sqrtf(sq);
}

/*
 * s32 muldiv(s32 a, s32 b, s32 c);
 *
 * calcsub.s:2096: abs inputs, 64-bit product (hi,lo), 32-iteration
 * shift-subtract division, sign from a ^ b ^ c.
 */
s32 muldiv(s32 a, s32 b, s32 c) {
    s32 sign = a ^ b ^ c;
    s32 i;
    u32 hi;
    u32 lo;
    s64 mult64;
    s32 ret;

    a = a < 0 ? -a : a;
    b = b < 0 ? -b : b;
    c = c < 0 ? -c : c;

    mult64 = (s64)a * (s64)b;
    lo = (u32)mult64;
    hi = (u32)(mult64 >> 32);

    ret = 0;
    for (i = 0; i < 33; i++) {
        ret <<= 1;
        if (hi >= (u32)c) {
            ret |= 1;
            hi -= (u32)c;
        }
        hi = (lo >> 31) | (hi << 1);
        lo <<= 1;
    }

    if (sign < 0) {
        return -ret;
    }
    return ret;
}

/*
 * void makeVect(s16 arg0, s16 arg1, s32 *arg2, s32 *arg3, s32 *arg4);
 *
 * calcsub.s:2158: mul (32-bit low half) then sra 15.
 *   *arg3 = -sinL(arg0)
 *   *arg2 = (sinL(arg1) * cosL(arg0)) >>s 15
 *   *arg4 = (cosL(arg1) * cosL(arg0)) >>s 15
 */
void makeVect(s16 arg0, s16 arg1, s32 *arg2, s32 *arg3, s32 *arg4) {
    s32 temp_s5 = -(s32)sinL(arg0);
    s32 temp_s6 = (s32)cosL(arg0);
    s32 temp_s7 = (s32)sinL(arg1);
    s32 temp_t3 = (s32)cosL(arg1);

    *arg3 = temp_s5;
    *arg2 = (temp_s7 * temp_s6) >> 15;
    *arg4 = (temp_t3 * temp_s6) >> 15;
}
