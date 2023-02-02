#include "neon.h"

int main() {
  print_int32x4x3_t(
    vld3q_lane_s32(
      set_int32_t_ptr(12),
      set_int32x4x3_t(),
      1));
  return 0;
}
