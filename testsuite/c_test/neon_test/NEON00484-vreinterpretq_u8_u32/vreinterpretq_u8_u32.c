#include "neon.h"

int main() {
  print_uint8x16_t(
    vreinterpretq_u8_u32(
      set_uint32x4_t()));
  return 0;
}
