#include "neon.h"

int main() {
  print_uint16x8_t(
    vaddl_high_u8(
      set_uint8x16_t(),
      set_uint8x16_t()));
  return 0;
}