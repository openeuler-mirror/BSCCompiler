#include "neon.h"

int main() {
  print_uint32x2_t(
    vreinterpret_u32_u16(
      set_uint16x4_t()));
  return 0;
}
