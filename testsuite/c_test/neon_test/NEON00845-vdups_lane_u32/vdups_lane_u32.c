#include "neon.h"

int main() {
  print_uint32_t(
    vdups_lane_u32(
      set_uint32x2_t(),
      1));
  return 0;
}
