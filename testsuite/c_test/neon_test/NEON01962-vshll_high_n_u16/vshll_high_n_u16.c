#include "neon.h"

int main() {
  print_uint32x4_t(
    vshll_high_n_u16(
      set_uint16x8_t(),
      1));
  return 0;
}
