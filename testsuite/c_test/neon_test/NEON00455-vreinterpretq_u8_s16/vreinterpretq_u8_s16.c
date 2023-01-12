#include "neon.h"

int main() {
  print_uint8x16_t(
    vreinterpretq_u8_s16(
      set_int16x8_t()));
  return 0;
}
