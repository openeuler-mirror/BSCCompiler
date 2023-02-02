#include "neon.h"

int main() {
  print_uint8x8x3_t(
    vld3_lane_u8(
      set_uint8_t_ptr(24),
      set_uint8x8x3_t(),
      1));
  return 0;
}
