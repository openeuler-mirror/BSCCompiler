#include "neon.h"

int main() {
  print_int32x2_t(
    vreinterpret_s32_u8(
      set_uint8x8_t()));
  return 0;
}
