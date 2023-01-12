#include "neon.h"

int main() {
  print_int16x8_t(
    vreinterpretq_s16_u16(
      set_uint16x8_t()));
  return 0;
}
