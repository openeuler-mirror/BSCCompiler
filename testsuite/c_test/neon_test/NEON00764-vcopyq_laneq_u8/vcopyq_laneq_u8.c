#include "neon.h"

int main() {
  print_uint8x16_t(
    vcopyq_laneq_u8(
      set_uint8x16_t(),
      1,
      set_uint8x16_t(),
      1));
  return 0;
}
