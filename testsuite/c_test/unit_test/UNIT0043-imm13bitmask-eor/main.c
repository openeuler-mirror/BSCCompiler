#include <stdint.h>
//test 0xfffffff7 is not bitmaskimm
uint32_t ui_0 = 4;
int16_t s_1 = 0;
uint64_t uli_3 = 4;
int8_t fn1(uint32_t ui_7) {
  int ui_8 = 0;
  int64_t li_10 = 1;
  for (s_1 = 9; s_1 <= 9; ui_0++)
    for (li_10 = -9; li_10 <= 6; li_10++) {
      uint64_t uli_13 = 7;
      uint32_t *ptr_14 = &ui_8;
      for (uli_13 = 1; uli_13 <= 8; uli_13++) {
        int32_t i_17 = 0;
        for (i_17 = 1; i_17 <= 2; ui_7++) {
          s_1 = 0 > 0 != (ui_7 = li_10) | 0;
          if (ui_7 ^= 0 ?: (uli_3 -= 0))
            for (li_10 = 7; li_10 <= 73; uli_3++)
              for (*ptr_14 = 7; *ptr_14 <= 1; *ptr_14++)
                ;
        }
      }
    }
}

int main() {
  return 0;
}
