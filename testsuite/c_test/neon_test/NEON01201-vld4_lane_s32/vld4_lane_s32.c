#include "neon.h"

int main() {
  print_int32x2x4_t(
    vld4_lane_s32(
      set_int32_t_ptr(8),
      set_int32x2x4_t(),
      1));
  return 0;
}
