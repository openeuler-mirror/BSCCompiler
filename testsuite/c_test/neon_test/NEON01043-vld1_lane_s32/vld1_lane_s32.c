#include "neon.h"

int main() {
  print_int32x2_t(
    vld1_lane_s32(
      set_int32_t_ptr(2),
      set_int32x2_t(),
      1));
  return 0;
}
