#include "neon.h"

int main() {
  print_uint32x4_t(
    vreinterpretq_u32_s16(
      set_int16x8_t()));
  return 0;
}
