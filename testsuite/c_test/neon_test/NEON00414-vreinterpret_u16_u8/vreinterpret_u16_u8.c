#include "neon.h"

int main() {
  print_uint16x4_t(
    vreinterpret_u16_u8(
      set_uint8x8_t()));
  return 0;
}
