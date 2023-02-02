#include "neon.h"

int main() {
  print_uint64x2_t(
    vld1q_lane_u64(
      set_uint64_t_ptr(2),
      set_uint64x2_t(),
      1));
  return 0;
}
