#include "neon.h"

int main() {
  print_uint16x8_t(
    vsubhn_high_u32(
      set_uint16x4_t(),
      set_uint32x4_t(),
      set_uint32x4_t()));
  return 0;
}
