#include "neon.h"

int main() {
  print_int16x8x2_t(
    vld2q_lane_s16(
      set_int16_t_ptr(16),
      set_int16x8x2_t(),
      1));
  return 0;
}
