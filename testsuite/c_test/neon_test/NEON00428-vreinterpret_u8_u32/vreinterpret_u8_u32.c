#include "neon.h"

int main() {
  print_uint8x8_t(
    vreinterpret_u8_u32(
      set_uint32x2_t()));
  return 0;
}
