#include "neon.h"

int main() {
  print_int64x1x3_t(
    vld3_lane_s64(
      set_int64_t_ptr(3),
      set_int64x1x3_t(),
      0));
  return 0;
}
