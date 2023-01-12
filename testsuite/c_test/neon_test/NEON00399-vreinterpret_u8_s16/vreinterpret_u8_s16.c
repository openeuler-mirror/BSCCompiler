#include "neon.h"

int main() {
  print_uint8x8_t(
    vreinterpret_u8_s16(
      set_int16x4_t()));
  return 0;
}
