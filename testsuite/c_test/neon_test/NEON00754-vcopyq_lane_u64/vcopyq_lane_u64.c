#include "neon.h"

int main() {
  print_uint64x2_t(
    vcopyq_lane_u64(
      set_uint64x2_t(),
      0,
      set_uint64x1_t(),
      0));
  return 0;
}
