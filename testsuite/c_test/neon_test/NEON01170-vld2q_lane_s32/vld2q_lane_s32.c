#include "neon.h"

int main() {
  print_int32x4x2_t(
    vld2q_lane_s32(
      set_int32_t_ptr(8),
      set_int32x4x2_t(),
      1));
  return 0;
}
