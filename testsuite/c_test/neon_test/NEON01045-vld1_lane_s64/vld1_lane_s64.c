#include "neon.h"

int main() {
  print_int64x1_t(
    vld1_lane_s64(
      set_int64_t_ptr(1),
      set_int64x1_t(),
      0));
  return 0;
}
