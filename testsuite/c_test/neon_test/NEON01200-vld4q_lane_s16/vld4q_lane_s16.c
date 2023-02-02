#include "neon.h"

int main() {
  print_int16x8x4_t(
    vld4q_lane_s16(
      set_int16_t_ptr(32),
      set_int16x8x4_t(),
      1));
  return 0;
}
