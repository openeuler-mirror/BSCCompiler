#include "neon.h"

int main() {
  print_uint16x8_t(
    vmlal_high_u8(
      set_uint16x8_t(),
      set_uint8x16_t(),
      set_uint8x16_t()));
  return 0;
}
