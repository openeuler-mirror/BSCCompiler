#include "neon.h"

int main() {
  print_int16x8_t(
    vreinterpretq_s16_u32(
      set_uint32x4_t()));
  return 0;
}
