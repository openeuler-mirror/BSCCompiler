#include "neon.h"

int main() {
  print_int16x4_t(
    vreinterpret_s16_u8(
      set_uint8x8_t()));
  return 0;
}
