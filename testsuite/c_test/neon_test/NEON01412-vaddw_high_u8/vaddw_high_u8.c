#include "neon.h"

int main() {
  print_uint16x8_t(
    vaddw_high_u8(
      set_uint16x8_t(),
      set_uint8x16_t()));
  return 0;
}