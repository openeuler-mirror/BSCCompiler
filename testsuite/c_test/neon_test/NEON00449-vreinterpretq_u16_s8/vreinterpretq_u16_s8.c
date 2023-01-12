#include "neon.h"

int main() {
  print_uint16x8_t(
    vreinterpretq_u16_s8(
      set_int8x16_t()));
  return 0;
}
