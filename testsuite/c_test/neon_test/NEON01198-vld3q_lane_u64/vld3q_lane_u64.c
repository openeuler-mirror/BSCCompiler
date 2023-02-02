#include "neon.h"

int main() {
  print_uint64x2x3_t(
    vld3q_lane_u64(
      set_uint64_t_ptr(6),
      set_uint64x2x3_t(),
      1));
  return 0;
}
