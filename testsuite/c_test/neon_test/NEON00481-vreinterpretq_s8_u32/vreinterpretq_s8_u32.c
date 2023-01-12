#include "neon.h"

int main() {
  print_int8x16_t(
    vreinterpretq_s8_u32(
      set_uint32x4_t()));
  return 0;
}
