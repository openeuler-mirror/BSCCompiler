#include "neon.h"

int main() {
  print_uint64_t(
    vgetq_lane_u64(
      set_uint64x2_t(),
      set_int()));
  return 0;
}
