#include "neon.h"

int main() {
  print_uint32x4_t(
    vmull_high_n_u16(
      set_uint16x8_t(),
      set_uint16_t()));
  return 0;
}
