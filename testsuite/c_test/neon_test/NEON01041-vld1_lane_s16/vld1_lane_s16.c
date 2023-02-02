#include "neon.h"

int main() {
  print_int16x4_t(
    vld1_lane_s16(
      set_int16_t_ptr(4),
      set_int16x4_t(),
      1));
  return 0;
}
