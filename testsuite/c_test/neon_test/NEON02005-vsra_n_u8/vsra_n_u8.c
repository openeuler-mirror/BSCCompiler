#include "neon.h"

int main() {
  print_uint8x8_t(
    vsra_n_u8(
      set_uint8x8_t(),
      set_uint8x8_t(),
      1));
  return 0;
}
