#ifndef __NEON_H__
#define __NEON_H__

#include <stdio.h>
#include <arm_neon.h>

#define VSET(a, lane, s, q) { a = vset##q##_lane_##s(lane, a, lane);}
#define VGET(a, lane, s, q) vget##q##_lane_##s(a, lane)

#define VSET_1(a, s, q) { VSET(a, 0, s, q);}
#define VSET_2(a, s, q) { VSET(a, 0, s, q); \
                          VSET(a, 1, s, q);}
#define VSET_4(a, s, q) { VSET(a, 0, s, q); \
                          VSET(a, 1, s, q); \
                          VSET(a, 2, s, q); \
                          VSET(a, 3, s, q);}
#define VSET_8(a, s, q) { VSET(a, 0, s, q); \
                          VSET(a, 1, s, q); \
                          VSET(a, 2, s, q); \
                          VSET(a, 3, s, q); \
                          VSET(a, 4, s, q); \
                          VSET(a, 5, s, q); \
                          VSET(a, 6, s, q); \
                          VSET(a, 7, s, q);}
#define VSET_16(a, s, q) { VSET(a, 0, s, q);  \
                           VSET(a, 1, s, q);  \
                           VSET(a, 2, s, q);  \
                           VSET(a, 3, s, q);  \
                           VSET(a, 4, s, q);  \
                           VSET(a, 5, s, q);  \
                           VSET(a, 6, s, q);  \
                           VSET(a, 7, s, q);  \
                           VSET(a, 8, s, q);  \
                           VSET(a, 9, s, q);  \
                           VSET(a, 10, s, q); \
                           VSET(a, 11, s, q); \
                           VSET(a, 12, s, q); \
                           VSET(a, 13, s, q); \
                           VSET(a, 14, s, q); \
                           VSET(a, 15, s, q);}

#define VPRTINT_1(a, s, q) { printf("%ld,", (long)VGET(a, 0, s, q));  \
                             printf("\n");}
#define VPRTINT_2(a, s, q) { printf("%ld,", (long)VGET(a, 0, s, q));  \
                             printf("%ld,", (long)VGET(a, 1, s, q));  \
                             printf("\n");}
#define VPRTINT_4(a, s, q) { printf("%ld,", (long)VGET(a, 0, s, q));  \
                             printf("%ld,", (long)VGET(a, 1, s, q));  \
                             printf("%ld,", (long)VGET(a, 2, s, q));  \
                             printf("%ld,", (long)VGET(a, 3, s, q));  \
                             printf("\n");}
#define VPRTINT_8(a, s, q) { printf("%ld,", (long)VGET(a, 0, s, q));  \
                             printf("%ld,", (long)VGET(a, 1, s, q));  \
                             printf("%ld,", (long)VGET(a, 2, s, q));  \
                             printf("%ld,", (long)VGET(a, 3, s, q));  \
                             printf("%ld,", (long)VGET(a, 4, s, q));  \
                             printf("%ld,", (long)VGET(a, 5, s, q));  \
                             printf("%ld,", (long)VGET(a, 6, s, q));  \
                             printf("%ld,", (long)VGET(a, 7, s, q));  \
                             printf("\n");}
#define VPRTINT_16(a, s, q) { printf("%ld,", (long)VGET(a, 0, s, q));  \
                              printf("%ld,", (long)VGET(a, 1, s, q));  \
                              printf("%ld,", (long)VGET(a, 2, s, q));  \
                              printf("%ld,", (long)VGET(a, 3, s, q));  \
                              printf("%ld,", (long)VGET(a, 4, s, q));  \
                              printf("%ld,", (long)VGET(a, 5, s, q));  \
                              printf("%ld,", (long)VGET(a, 6, s, q));  \
                              printf("%ld,", (long)VGET(a, 7, s, q));  \
                              printf("%ld,", (long)VGET(a, 8, s, q));  \
                              printf("%ld,", (long)VGET(a, 9, s, q));  \
                              printf("%ld,", (long)VGET(a, 10, s, q));  \
                              printf("%ld,", (long)VGET(a, 11, s, q));  \
                              printf("%ld,", (long)VGET(a, 12, s, q));  \
                              printf("%ld,", (long)VGET(a, 13, s, q));  \
                              printf("%ld,", (long)VGET(a, 14, s, q));  \
                              printf("%ld,", (long)VGET(a, 15, s, q));  \
                              printf("\n");}

#define FUNC_DEF(t, n, s, q)                          \
void print_##t(t a) { VPRTINT_##n (a, s, q); }        \
t set_##t() { t a; VSET_##n (a, s, q); return a; }

FUNC_DEF(int8x8_t, 8, s8, )
FUNC_DEF(int8x16_t, 16, s8, q)
FUNC_DEF(int16x4_t, 4, s16, )
FUNC_DEF(int16x8_t, 8, s16, q)
FUNC_DEF(int32x2_t, 2, s32, )
FUNC_DEF(int32x4_t, 4, s32, q)
FUNC_DEF(int64x1_t, 1, s64, )
FUNC_DEF(int64x2_t, 2, s64, q)
FUNC_DEF(uint8x8_t, 8, u8, )
FUNC_DEF(uint8x16_t, 16, u8, q)
FUNC_DEF(uint16x4_t, 4, u16, )
FUNC_DEF(uint16x8_t, 8, u16, q)
FUNC_DEF(uint32x2_t, 2, u32, )
FUNC_DEF(uint32x4_t, 4, u32, q)
FUNC_DEF(uint64x1_t, 1, u64, )
FUNC_DEF(uint64x2_t, 2, u64, q)

#undef FUNC_DEF

#define FUNC_DEF(t, ot)                                                     \
void print_##t(t a) { print_##ot(a.val[0]); print_##ot(a.val[1]); }         \
t set_##t() {t a; a.val[0] = set_##ot(); a.val[1] = set_##ot(); return a; }

FUNC_DEF(int8x8x2_t, int8x8_t)
FUNC_DEF(int8x16x2_t, int8x16_t)
FUNC_DEF(int16x4x2_t, int16x4_t)
FUNC_DEF(int16x8x2_t, int16x8_t)
FUNC_DEF(int32x2x2_t, int32x2_t)
FUNC_DEF(int32x4x2_t, int32x4_t)
FUNC_DEF(uint8x8x2_t, uint8x8_t)
FUNC_DEF(uint8x16x2_t, uint8x16_t)
FUNC_DEF(uint16x4x2_t, uint16x4_t)
FUNC_DEF(uint16x8x2_t, uint16x8_t)
FUNC_DEF(uint32x2x2_t, uint32x2_t)
FUNC_DEF(uint32x4x2_t, uint32x4_t)

#undef FUNC_DEF

#define FUNC_DEF(t)                                 \
void print_##t(t a) { printf("%ld\n", (long)a); }   \
t set_##t() { return 1; }

FUNC_DEF(int8_t)
FUNC_DEF(int16_t)
FUNC_DEF(int32_t)
FUNC_DEF(int64_t)
FUNC_DEF(uint8_t)
FUNC_DEF(uint16_t)
FUNC_DEF(uint32_t)
FUNC_DEF(uint64_t)

#undef FUNC_DEF

void print_int(int a) {
  printf("%ld\n", (long)a);
}
#define set_int() (0)

#endif /* __NEON_H__ */