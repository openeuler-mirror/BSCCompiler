#include "neon.h"

int main() {
  print_uint64x1_t(
    vld1_lane_u64(
      set_uint64_t_ptr(1),
      set_uint64x1_t(),
      0));
  return 0;
}
