#include "neon.h"

int main() {
  print_uint64x1x2_t(
    vld2_lane_u64(
      set_uint64_t_ptr(2),
      set_uint64x1x2_t(),
      0));
  return 0;
}
