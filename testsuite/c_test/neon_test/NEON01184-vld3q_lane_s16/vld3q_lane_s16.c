#include "neon.h"

int main() {
  print_int16x8x3_t(
    vld3q_lane_s16(
      set_int16_t_ptr(24),
      set_int16x8x3_t(),
      1));
  return 0;
}
