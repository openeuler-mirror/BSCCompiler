#include "neon.h"

int main() {
  print_uint8x16_t(
    vqshrn_high_n_u16(
      set_uint8x8_t(),
      set_uint16x8_t(),
      1));
  return 0;
}
