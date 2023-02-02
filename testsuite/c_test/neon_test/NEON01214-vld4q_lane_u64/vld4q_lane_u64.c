#include "neon.h"

int main() {
  print_uint64x2x4_t(
    vld4q_lane_u64(
      set_uint64_t_ptr(8),
      set_uint64x2x4_t(),
      1));
  return 0;
}
