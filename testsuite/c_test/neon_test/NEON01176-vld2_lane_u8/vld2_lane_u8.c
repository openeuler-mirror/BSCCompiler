#include "neon.h"

int main() {
  print_uint8x8x2_t(
    vld2_lane_u8(
      set_uint8_t_ptr(16),
      set_uint8x8x2_t(),
      1));
  return 0;
}
