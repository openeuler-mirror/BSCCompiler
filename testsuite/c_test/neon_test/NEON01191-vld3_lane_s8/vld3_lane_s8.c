#include "neon.h"

int main() {
  print_int8x8x3_t(
    vld3_lane_s8(
      set_int8_t_ptr(24),
      set_int8x8x3_t(),
      1));
  return 0;
}
