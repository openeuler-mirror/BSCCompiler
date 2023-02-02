#include "neon.h"

int main() {
  print_uint32x4_t(
    vld1q_lane_u32(
      set_uint32_t_ptr(4),
      set_uint32x4_t(),
      1));
  return 0;
}
