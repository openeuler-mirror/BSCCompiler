#include "neon.h"

int main() {
  print_uint8x16_t(
    vqmovun_high_s16(
      set_uint8x8_t(),
      set_int16x8_t()));
  return 0;
}
