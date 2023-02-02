#include "neon.h"

int main() {
  print_uint64x2_t(
    vmull_high_lane_u32(
      set_uint32x4_t(),
      set_uint32x2_t(),
      1));
  return 0;
}
