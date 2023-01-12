#include "neon.h"

int main() {
  print_int16x8_t(
    vreinterpretq_s16_u8(
      set_uint8x16_t()));
  return 0;
}
