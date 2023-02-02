#include "neon.h"

int main() {
  print_uint64x1x3_t(
    vld3_lane_u64(
      set_uint64_t_ptr(3),
      set_uint64x1x3_t(),
      0));
  return 0;
}
