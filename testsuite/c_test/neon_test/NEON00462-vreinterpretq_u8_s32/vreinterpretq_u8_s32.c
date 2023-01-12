#include "neon.h"

int main() {
  print_uint8x16_t(
    vreinterpretq_u8_s32(
      set_int32x4_t()));
  return 0;
}
