#include "neon.h"

int main() {
  print_uint64x2_t(
    vmlal_high_laneq_u32(
      set_uint64x2_t(),
      set_uint32x4_t(),
      set_uint32x4_t(),
      1));
  return 0;
}
