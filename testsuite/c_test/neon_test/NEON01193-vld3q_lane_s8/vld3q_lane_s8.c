#include "neon.h"

int main() {
  print_int8x16x3_t(
    vld3q_lane_s8(
      set_int8_t_ptr(48),
      set_int8x16x3_t(),
      1));
  return 0;
}
