#include "neon.h"

int main() {
  print_uint32x2_t(
    vreinterpret_u32_u8(
      set_uint8x8_t()));
  return 0;
}
