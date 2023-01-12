#include "neon.h"

int main() {
  print_uint8x16_t(
    vreinterpretq_u8_u16(
      set_uint16x8_t()));
  return 0;
}
