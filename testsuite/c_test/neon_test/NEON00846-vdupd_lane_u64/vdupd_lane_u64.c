#include "neon.h"

int main() {
  print_uint64_t(
    vdupd_lane_u64(
      set_uint64x1_t(),
      set_int()));
  return 0;
}
