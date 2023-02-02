#include "neon.h"

int main() {
  print_uint64_t(
    vget_lane_u64(
      set_uint64x1_t(),
      0));
  return 0;
}
