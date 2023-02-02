#include "neon.h"

int main() {
  print_int16x8_t(
    vld1q_lane_s16(
      set_int16_t_ptr(8),
      set_int16x8_t(),
      1));
  return 0;
}
