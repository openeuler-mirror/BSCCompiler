#include "neon.h"

int main() {
  print_uint8x16_t(
    vcopyq_lane_u8(
      set_uint8x16_t(),
      1,
      set_uint8x8_t(),
      1));
  return 0;
}