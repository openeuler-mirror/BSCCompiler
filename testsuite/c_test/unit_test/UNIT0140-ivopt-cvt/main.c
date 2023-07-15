#include "csmith.h"
int32_t g_4, g_1330 = 8;
int32_t *g_41 = 0;
int32_t **g_40 = &g_41;
const int32_t g_69 = 2;
const int32_t *g_68 = &g_69;
uint64_t g_93 = 2;
uint16_t g_106[];
uint32_t g_129 = 1;
int32_t *g_218 = 0;
int64_t g_454 = 0;
int32_t g_565 = 1;
int8_t g_571[][1];
uint8_t g_614 = 0;
const int32_t **g_866 = &g_68;
int16_t func_5();
int16_t func_13();
int32_t **func_16(uint32_t p_17, uint8_t p_18, int8_t p_19);
uint8_t func_20();
uint16_t func_30();
uint8_t func_34(int32_t **p_35, const uint64_t p_36, uint16_t p_37,
                int32_t *p_38, int32_t *p_39);
int32_t *func_49();
const uint64_t func_1() {
  int32_t *l_11 = &g_4;
  int8_t l_29 = 5;
  uint8_t l_936[0];
  int8_t l_1532 = 0;
  if (func_5(
          g_4, l_11,
          func_13(
              func_16(
                  g_4,
                  func_20(safe_add_func_uint8_t_u_u(
                              safe_lshift_func_uint32_t_u_s(
                                  l_29 | func_30(g_4, safe_sub_func_int16_t_s_s(
                                                          g_4 &
                                                              func_34(g_40, 0,
                                                                      0, 0, 0) &
                                                              8,
                                                          g_614)),
                                  5),
                              1),
                          0, 0, 0),
                  g_571[7][0]),
              l_936),
          g_1330, g_866),
      l_1532)
    ;
  return 0;
}
int16_t func_5() { return 0; }
int16_t func_13() { return 0; }
int32_t **func_16(uint32_t p_17, uint8_t p_18, int8_t p_19) {
  int32_t **l_935 = &g_218;
  for (0; 0; g_565--)
    ;
  return l_935;
}
uint8_t func_20() { return g_454; }
uint16_t func_30() { return g_93; }
uint8_t func_34(int32_t **p_35, const uint64_t p_36, uint16_t p_37,
                int32_t *p_38, int32_t *p_39) {
  int32_t l_408 = 4;
  uint16_t l_409 = 3;
  uint16_t *l_420 = &g_106[0];
  int32_t l_421;
  for (l_409 = 0; l_409 <= 3; l_409++) {
    int32_t l_480 = 4;
    for (g_129 = 0; l_480 >= 0; l_480--) {
      int32_t *l_623 = &l_408;
      *p_35 = func_49(
          l_623, &l_623, l_421,
          p_37 < (*l_623 = safe_sub_func_uint8_t_u_u((*l_420)-- <= 0, p_36)) !=
              1,
          p_36);
    }
  }
  return p_36;
}
int32_t *func_49() { return 0; }
void main() { func_1(); }
