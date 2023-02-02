#include "neon.h"

int main() {
  print_int64x2x2_t(
    vld2q_lane_s64(
      set_int64_t_ptr(4),
      set_int64x2x2_t(),
      1));
  return 0;
}
