#include "neon.h"

int main() {
  print_uint64x1x4_t(
    vld4_lane_u64(
      set_uint64_t_ptr(4),
      set_uint64x1x4_t(),
      0));
  return 0;
}
