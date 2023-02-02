#include "neon.h"

int main() {
  print_uint32x2x4_t(
    vld4_lane_u32(
      set_uint32_t_ptr(8),
      set_uint32x2x4_t(),
      1));
  return 0;
}
