#include "neon.h"

int main() {
  print_int32x2_t(
    vdup_lane_s32(
      set_int32x2_t(),
      1));
  return 0;
}
