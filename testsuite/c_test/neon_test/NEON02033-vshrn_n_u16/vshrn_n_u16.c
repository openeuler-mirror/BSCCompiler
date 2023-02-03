#include "neon.h"

int main() {
  print_uint8x8_t(
    vshrn_n_u16(
      set_uint16x8_t(),
      1));
  return 0;
}
