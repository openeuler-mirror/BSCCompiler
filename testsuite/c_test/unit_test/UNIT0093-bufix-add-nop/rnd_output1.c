#include <stdint.h>
#include <limits.h>
#include "rnd_globals.h"

/* --- FORWARD DECLARATIONS --- */
int32_t func_1 (void);
int32_t func_10 (int16_t p_11, uint16_t p_12, int32_t p_13);
uint32_t func_14 (uint16_t p_15, int32_t p_16, uint32_t p_17, int16_t p_18, int8_t p_19);
uint64_t func_30 (int16_t p_31);
int64_t func_36 (int32_t p_37);
int64_t func_40 (int16_t p_41, int64_t * p_42, int64_t * p_43, int64_t * p_44);
int16_t func_46 (uint32_t p_47, int16_t p_48, uint32_t p_49, int64_t * p_50, int8_t p_51);
int64_t * func_53 (int64_t * p_54, int64_t * p_55, uint16_t p_56);
union U1 * const func_61 (int32_t p_62, int64_t * p_63, union U1  p_64, int32_t p_65);
union U1 * func_66 (const union U1 * p_67);

volatile int32_t g_2 = 0x35F394A2L; /* VOLATILE GLOBAL g_2 */
int32_t g_7 = 0x2D76715FL;
uint64_t g_84 = 4UL;
uint32_t g_94[4][2] = {{0xA3421904L, 0xA3421904L},{0xA3421904L, 0xA3421904L},{0xA3421904L, 0xA3421904L},
                       {0xA3421904L, 0xA3421904L}};
uint64_t g_103 = 0x7EEE40B14FAB0846LL;
union U1 *g_108[2][2] = {{&g_69, &g_69},{&g_69, &g_69}};
union U1 g_110 = {7L};
const union U1 *g_109[5] = {&g_110, &g_110, &g_110, &g_110, &g_110};
int8_t g_144 = 0x2BL;
int8_t g_323 = 0xA5L;
uint32_t g_412[2][3] = {{0xF7C10BBDL,4UL, 0xF7C10BBDL},{0xF7C10BBDL,4UL, 0xF7C10BBDL}};
volatile int32_t ** volatile g_415 = &g_416; /* VOLATILE GLOBAL g_415 */
volatile uint64_t *g_450[1] = {&g_451};
volatile uint64_t * volatile *g_449 = &g_450[0];
union U1 **g_487[9][6][3] = {{{&g_108[0][0], &g_108[0][0], &g_108[0][1]},{&g_108[0][0], &g_108[1][1], &g_108[0][1]},
                             {&g_108[1][1], &g_108[0][0], &g_108[0][0]},{&g_108[0][0], &g_108[0][0], &g_108[0][0]},
                             {&g_108[0][0], &g_108[1][1],(void*)0},{&g_108[0][0], &g_108[0][0], &g_108[1][1]}},
                             {{&g_108[0][0], &g_108[0][1], &g_108[0][0]},{&g_108[0][0], &g_108[0][1], &g_108[0][1]},
                             {&g_108[0][0], &g_108[0][0], &g_108[0][0]},{&g_108[0][0], &g_108[1][1], &g_108[0][0]},
                             {&g_108[1][1], &g_108[0][0],(void*)0},{&g_108[0][0], &g_108[0][0],(void*)0}},
                             {{&g_108[1][0], &g_108[1][1],(void*)0},{&g_108[0][1], &g_108[0][0], &g_108[0][0]},
                             {&g_108[1][1],(void*)0, &g_108[0][0]},{&g_108[1][1], &g_108[1][1], &g_108[0][1]},
                             {(void*)0, (void*)0, (void*)0},{&g_108[0][0], &g_108[0][0], &g_108[0][0]}},
                             {{&g_108[1][1], &g_108[0][0], &g_108[0][0]},{&g_108[0][0], &g_108[1][1],(void*)0},
                             {&g_108[1][1], &g_108[0][1], &g_108[1][1]},{&g_108[0][1], &g_108[0][0], &g_108[1][1]},
                             {&g_108[0][0], &g_108[0][1],(void*)0},{&g_108[0][0], &g_108[1][1], &g_108[0][0]}},
                             {{&g_108[0][0], &g_108[0][0], &g_108[1][1]},{&g_108[0][1], &g_108[0][0], &g_108[0][0]},
                             {&g_108[0][0],(void*)0, &g_108[0][0]},{&g_108[1][1], &g_108[0][0], &g_108[1][1]},
                             {&g_108[0][0], &g_108[0][0], &g_108[0][0]},{&g_108[1][1], &g_108[1][0],(void*)0}},
                             {{&g_108[0][0], &g_108[0][0], &g_108[1][1]},{&g_108[1][1], &g_108[0][0], &g_108[1][1]},
                             {&g_108[0][0],(void*)0, (void*)0},{&g_108[1][1], &g_108[1][1], &g_108[0][0]},
                             {&g_108[0][0], &g_108[0][1], &g_108[0][0]},{&g_108[1][1], &g_108[1][1],(void*)0}},
                             {{&g_108[0][0], &g_108[1][1],(void*)0},{&g_108[0][1], &g_108[0][1], &g_108[0][0]},
                             {&g_108[0][0], &g_108[1][1], &g_108[0][0]},{&g_108[0][0],(void*)0, &g_108[0][0]},
                             {&g_108[0][0], &g_108[0][0], &g_108[0][0]},{&g_108[0][1], &g_108[0][0], &g_108[0][0]}},
                             {{&g_108[1][1], &g_108[1][0], &g_108[0][0]},{&g_108[0][0], &g_108[0][0], &g_108[0][0]},
                             {&g_108[1][1], &g_108[0][0],(void*)0},{&g_108[0][0],(void*)0, (void*)0},
                             {&g_108[0][0], &g_108[0][0], &g_108[0][0]},{&g_108[1][1], &g_108[0][0], &g_108[0][0]}},
                             {{&g_108[0][0], &g_108[1][1],(void*)0},{&g_108[1][1], &g_108[0][1], &g_108[1][1]},
                             {&g_108[0][1], &g_108[0][0], &g_108[1][1]},{&g_108[0][0], &g_108[0][1],(void*)0},
                             {&g_108[0][0], &g_108[1][1], &g_108[0][0]},{&g_108[0][0], &g_108[0][0], &g_108[1][1]}}};
volatile int64_t g_579 = 1L; /* VOLATILE GLOBAL g_579 */
int32_t *g_585 = &g_3;
volatile uint64_t g_613[5][4][10] = {{{0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL,
                                       0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL, 0xC192EBC3FCAF32AFLL,
                                       0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL, 0x4D83A56AE498C40FLL,
                                       0xA68B84ED5EE4F31DLL},
                                      {0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL,
                                       0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL, 0xC192EBC3FCAF32AFLL,
                                       0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL, 0x4D83A56AE498C40FLL,
                                       0xA68B84ED5EE4F31DLL},{0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL,
                                       0x4D83A56AE498C40FLL, 0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL,
                                       0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL,
                                       0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL},
                                      {0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL,
                                       0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL, 0xC192EBC3FCAF32AFLL,
                                       0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL, 0x4D83A56AE498C40FLL,
                                       0xA68B84ED5EE4F31DLL}},
                                     {{0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL,
                                       0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL, 0xC192EBC3FCAF32AFLL,
                                       0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL, 0x4D83A56AE498C40FLL,
                                       0xA68B84ED5EE4F31DLL},
                                      {0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL,
                                       0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL, 0xC192EBC3FCAF32AFLL,
                                       0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL, 0x4D83A56AE498C40FLL,
                                       0xA68B84ED5EE4F31DLL},
                                      {0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL,
                                       0x4D83A56AE498C40FLL, 0xA68B84ED5EE4F31DLL, 0xC192EBC3FCAF32AFLL,
                                       0xA68B84ED5EE4F31DLL, 0x4D83A56AE498C40FLL, 0x4D83A56AE498C40FLL,
                                       0xA68B84ED5EE4F31DLL},
                                      {0xC192EBC3FCAF32AFLL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL}},
                                     {{0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL}},
                                     {{0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL}},
                                     {{0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL},
                                      {0x55418A44EE6E938FLL, 18446744073709551615UL, 0xA68B84ED5EE4F31DLL,
                                       0xA68B84ED5EE4F31DLL, 18446744073709551615UL, 0x55418A44EE6E938FLL,
                                       18446744073709551615UL, 0xA68B84ED5EE4F31DLL, 0xA68B84ED5EE4F31DLL,
                                       18446744073709551615UL}}};
/* ------------------------------------------ */
/* 
 * reads : g_423 g_2 g_329 g_109 g_69.f0 g_3 g_94 g_323 g_144 g_129 g_449 g_185 g_125 g_110.f0 g_467
 * writes: g_423 g_323 g_144 g_129 g_185 g_110.f0 g_125 g_468
 */
int32_t func_10 (int16_t p_11, uint16_t p_12, int32_t p_13) {
    /* block id: 249 */
    int32_t *l_420 = &g_69.f0;
    int32_t *l_421 = &g_110.f0;
    int32_t *l_422[3][7] = {{(void*)0, (void*)0, &g_110.f0, (void*)0, (void*)0, &g_110.f0, (void*)0},
                            {&g_69.f0, &g_69.f0, &g_69.f0, &g_69.f0, &g_69.f0, &g_69.f0, &g_69.f0},
                            {&g_110.f0, (void*)0, &g_110.f0, &g_110.f0, (void*)0, &g_110.f0, &g_110.f0}};
    uint32_t l_432 = 0xD1486B35L;
    union U1 **l_435 = &g_108[1][1];
    int8_t l_442 = 0L;
    int32_t l_469[4] = {0x173F234AL, 0x173F234AL, 0x173F234AL, 0x173F234AL};
    int i, j;
    --g_423;
    if (((safe_mul_func_uint8_t_u_u(((safe_mul_func_int8_t_s_s(p_11, ((safe_rshift_func_int16_t_s_s(((l_432 || 3UL),
        g_2), 15)), (4294967292UL && (safe_sub_func_uint64_t_u_u((p_11 ^ (((*g_329) ==
        (((p_11 == (l_435 != &g_109[0])) & 0xF2L), &g_69)) & (*l_420))), g_3)))))) > g_94[2][0]), 1UL)) | p_13)) {
            /* block id: 251 */
        int32_t l_438 = 0x3BE4CBB7L;
        for (p_12 = 2; (p_12 < 60); ++p_12) {
            /* block id: 254 */
            for (g_323 = 3; (g_323 >= 0); g_323 -= 1) {
                /* block id: 257 */
                uint32_t l_439 = 0x377B733BL;
                l_439++;
                if (l_438)
                    break;
            }
        }
        for (g_144 = 2; (g_144 >= 0); g_144 -= 1) {
            /* block id: 264 */
            int16_t l_463 = 0L;
            l_442 = l_438;
            for (g_129 = 0; (g_129 <= 2); g_129 += 1) {
                /* block id: 268 */
                uint16_t *l_458 = &g_185;
                int32_t l_464[7];
                int i, j;
                for (i = 0; i < 7; i++)
                    l_464[i] = 4L;
                (*l_421) &= (((safe_mul_func_uint8_t_u_u(((safe_mod_func_int16_t_s_s((safe_rshift_func_int64_t_s_s((
                            g_449 != (void*)0), 31)), l_438)) ==
                            (safe_sub_func_uint8_t_u_u(((((safe_div_func_uint64_t_u_u((+(+((*l_458)++))),
                            (safe_lshift_func_int8_t_s_s((l_463 && g_125), (l_464[6] > l_463))))) &&
                            (((((void*)0 != &g_144) > l_463), p_13) | (-1L))), g_3) <= l_438), p_12))), 0UL))
                            & (*l_420)), l_438);
            }
        }
        for (g_125 = 0; (g_125 <= 20); ++g_125) {
            /* block id: 275 */
            if (l_438)
                break;
        }
    }
    else {
        /* block id: 278 */
        (*g_467) = l_435;
    }
    return l_469[2];
}

/* ------------------------------------------ */
/* 
 * reads :
 * writes:
 */
uint64_t func_30 (int16_t p_31) {
    /* block id: 122 */
    uint32_t l_242 = 0xC49BE158L;
    return l_242;
}

/* ------------------------------------------ */
/* 
 * reads : g_69.f0 g_3 g_57 g_58 g_7 g_101 g_98
 * writes: g_69.f0 g_84 g_103 g_98
 */
union U1 * func_66 (const union U1 * p_67) {
    /* block id: 13 */
    const int32_t l_82 = 0xF4B1A25DL;
    uint64_t *l_83 = &g_84;
    uint16_t l_87 = 0xF013L;
    uint32_t *l_93[1];
    int32_t l_95 = 0x81E2E8C4L;
    int32_t l_96 = (-1L);
    int16_t *l_97[5] = {&g_98[3], &g_98[3], &g_98[3], &g_98[3], &g_98[3]};
    int32_t l_99 = 0x0883475FL;
    int32_t l_100 = 8L;
    uint64_t *l_102 = &g_103;
    int32_t *l_104 = &g_69.f0;
    int i;
    for (i = 0; i < 1; i++)
        l_93[i] = &g_94[0][1];
lbl_105:
    for (g_69.f0 = (-20); (g_69.f0 <= 24); g_69.f0 = safe_add_func_uint64_t_u_u(g_69.f0, 9)) {
        /* block id: 16 */
        uint16_t l_72 = 65535UL;
        ++l_72;
    }
    (*l_104) = ((((g_98[3] ^= (((*l_102) = (((+(safe_div_func_int16_t_s_s((safe_add_func_int64_t_s_s((0x4A3CL >= (g_3 &
        (safe_lshift_func_int8_t_s_u((-7L), 4)))), (((l_82, (((*l_83) = 1UL) !=
        (safe_div_func_uint8_t_u_u(l_87, (l_100 = (~(((l_99 = (l_96 =
        ((safe_mod_func_uint32_t_u_u(l_87, l_87)) || ((safe_add_func_uint32_t_u_u(((l_95 =
        ((((l_82 == l_87) | (*g_57)) & g_7) > 0x6771L)) == l_82), 4294967289UL)) <= g_58)))) != 0x0B1EL) ||
        l_99))))))), (void*)0) == (void*)0))), g_101))) == l_82) < l_87)), g_58)) && l_96), l_102) != l_102);
    if (l_87)
        goto lbl_105;
    return &g_69;
}


