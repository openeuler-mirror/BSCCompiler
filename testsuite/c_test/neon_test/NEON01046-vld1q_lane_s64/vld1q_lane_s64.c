#include "neon.h"

int main() {
  print_int64x2_t(
    vld1q_lane_s64(
      set_int64_t_ptr(2),
      set_int64x2_t(),
      1));
  return 0;
}
