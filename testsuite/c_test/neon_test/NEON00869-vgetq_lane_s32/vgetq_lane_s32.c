#include "neon.h"

int main() {
  print_int32_t(
    vgetq_lane_s32(
      set_int32x4_t(),
      1));
  return 0;
}
