#include "neon.h"

int main() {
  print_uint64x2_t(
    vmlal_high_lane_u32(
      set_uint64x2_t(),
      set_uint32x4_t(),
      set_uint32x2_t(),
      1));
  return 0;
}
