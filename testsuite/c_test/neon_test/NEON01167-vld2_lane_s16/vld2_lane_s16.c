#include "neon.h"

int main() {
  print_int16x4x2_t(
    vld2_lane_s16(
      set_int16_t_ptr(8),
      set_int16x4x2_t(),
      1));
  return 0;
}
