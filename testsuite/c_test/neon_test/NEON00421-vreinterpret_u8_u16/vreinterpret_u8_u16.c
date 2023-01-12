#include "neon.h"

int main() {
  print_uint8x8_t(
    vreinterpret_u8_u16(
      set_uint16x4_t()));
  return 0;
}
