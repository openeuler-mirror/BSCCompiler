#include "neon.h"

int main() {
  print_uint16x8_t(
    vshll_high_n_u8(
      set_uint8x16_t(),
      1));
  return 0;
}
