#include "neon.h"

int main() {
  print_uint32x2x2_t(
    vld2_lane_u32(
      set_uint32_t_ptr(4),
      set_uint32x2x2_t(),
      1));
  return 0;
}
