#include "neon.h"

int main() {
  print_int16x8_t(
    vsubw_high_s8(
      set_int16x8_t(),
      set_int8x16_t()));
  return 0;
}
