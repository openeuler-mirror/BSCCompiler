#include "neon.h"

int main() {
  print_int32x4_t(
    vld1q_lane_s32(
      set_int32_t_ptr(4),
      set_int32x4_t(),
      1));
  return 0;
}
