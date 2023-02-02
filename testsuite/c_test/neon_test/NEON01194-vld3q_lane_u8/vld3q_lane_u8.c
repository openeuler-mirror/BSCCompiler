#include "neon.h"

int main() {
  print_uint8x16x3_t(
    vld3q_lane_u8(
      set_uint8_t_ptr(48),
      set_uint8x16x3_t(),
      1));
  return 0;
}
