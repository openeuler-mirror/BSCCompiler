#include "neon.h"

int main() {
  print_int8x16_t(
    vreinterpretq_s8_u16(
      set_uint16x8_t()));
  return 0;
}
