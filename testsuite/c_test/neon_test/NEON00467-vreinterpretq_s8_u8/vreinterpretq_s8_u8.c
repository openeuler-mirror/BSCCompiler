#include "neon.h"

int main() {
  print_int8x16_t(
    vreinterpretq_s8_u8(
      set_uint8x16_t()));
  return 0;
}
