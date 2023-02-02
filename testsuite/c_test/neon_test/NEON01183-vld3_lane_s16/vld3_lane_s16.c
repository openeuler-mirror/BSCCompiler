#include "neon.h"

int main() {
  print_int16x4x3_t(
    vld3_lane_s16(
      set_int16_t_ptr(12),
      set_int16x4x3_t(),
      1));
  return 0;
}
