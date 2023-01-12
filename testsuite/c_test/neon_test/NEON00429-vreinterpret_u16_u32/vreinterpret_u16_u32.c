#include "neon.h"

int main() {
  print_uint16x4_t(
    vreinterpret_u16_u32(
      set_uint32x2_t()));
  return 0;
}
