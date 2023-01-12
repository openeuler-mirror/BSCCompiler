#include "neon.h"

int main() {
  print_int32x4_t(
    vreinterpretq_s32_u8(
      set_uint8x16_t()));
  return 0;
}
