#include "neon.h"

int main() {
  print_int64x1x2_t(
    vld2_lane_s64(
      set_int64_t_ptr(2),
      set_int64x1x2_t(),
      0));
  return 0;
}
