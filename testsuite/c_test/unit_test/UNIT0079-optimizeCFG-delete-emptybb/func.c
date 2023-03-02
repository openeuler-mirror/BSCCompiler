#include <stdint.h>
uint32_t ui_0 = 9;
uint8_t uc_1 = 0;
uint64_t uli_3 = 0;
int16_t s_6 = 3;
uint16_t us_7 = 0;
uint8_t uc_8 = 1;
uint32_t fn1(int32_t i_12) {
  int li_13 = 0, uli_11 = 0;
  uint8_t *ptr_14 = &uc_1;
  int32_t i_15 = 2;
  int64_t *ptr_16 = &li_13;
  for (*ptr_14 = 6; *ptr_14 <= 82; uli_11++) {
    int16_t *ptr_17 = &s_6;
  lblEB190337:
    for (us_7 = 4; uc_8 <= 9; i_15++)
      for (ui_0 = 30; ui_0 <= 68; ui_0++) {
        uint32_t *ptr_21 = &ui_0;
        int64_t li_23 = 2;
        if (((2 &&*ptr_17 ? uc_1 ^= 0 : 1 && *ptr_21)
                 ? 0 >= uli_3 ? 0 : *ptr_17 == li_23
                 : 0) > (i_12 < 0 & (*ptr_17 ? *ptr_16 : uc_8) || 0)) {
          uint32_t **ptr_24 = &ptr_21;
        }
      }
    for (*ptr_17 = 5; *ptr_14 <= 8; *ptr_14 = 1)
      ;
  }
}

int main() {
  return 0;
}
