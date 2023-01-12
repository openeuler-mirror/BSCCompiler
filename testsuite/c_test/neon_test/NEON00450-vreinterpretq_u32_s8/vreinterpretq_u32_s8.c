#include "neon.h"

int main() {
  print_uint32x4_t(
    vreinterpretq_u32_s8(
      set_int8x16_t()));
  return 0;
}
