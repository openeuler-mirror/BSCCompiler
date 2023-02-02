#include "neon.h"

int main() {
  print_uint8x16x2_t(
    vld2q_lane_u8(
      set_uint8_t_ptr(32),
      set_uint8x16x2_t(),
      1));
  return 0;
}
