#include "neon.h"

int main() {
  print_uint16x8_t(
    vreinterpretq_u16_s16(
      set_int16x8_t()));
  return 0;
}
