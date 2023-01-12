#include "neon.h"

int main() {
  print_uint8x8_t(
    vreinterpret_u8_s32(
      set_int32x2_t()));
  return 0;
}
