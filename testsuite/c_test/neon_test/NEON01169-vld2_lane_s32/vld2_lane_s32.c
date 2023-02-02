#include "neon.h"

int main() {
  print_int32x2x2_t(
    vld2_lane_s32(
      set_int32_t_ptr(4),
      set_int32x2x2_t(),
      1));
  return 0;
}
