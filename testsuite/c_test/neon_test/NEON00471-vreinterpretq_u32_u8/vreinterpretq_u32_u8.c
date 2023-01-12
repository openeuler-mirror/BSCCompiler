#include "neon.h"

int main() {
  print_uint32x4_t(
    vreinterpretq_u32_u8(
      set_uint8x16_t()));
  return 0;
}
