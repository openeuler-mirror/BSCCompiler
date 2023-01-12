#include "neon.h"

int main() {
  print_int32_t(
    vget_lane_s32(
      set_int32x2_t(),
      set_int()));
  return 0;
}
