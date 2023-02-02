#include "neon.h"

int main() {
  print_int8x16_t(
    vld1q_lane_s8(
      set_int8_t_ptr(16),
      set_int8x16_t(),
      1));
  return 0;
}
