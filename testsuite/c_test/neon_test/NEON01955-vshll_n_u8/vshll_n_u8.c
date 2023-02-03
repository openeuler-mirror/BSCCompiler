#include "neon.h"

int main() {
  print_uint16x8_t(
    vshll_n_u8(
      set_uint8x8_t(),
      1));
  return 0;
}
