#include "neon.h"

int main() {
  print_uint8x8_t(
    vcopy_laneq_u8(
      set_uint8x8_t(),
      set_int(),
      set_uint8x16_t(),
      set_int()));
  return 0;
}
