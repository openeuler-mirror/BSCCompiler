#include "neon.h"

int main() {
  print_int32x2_t(
    vmul_lane_s32(
      set_int32x2_t(),
      set_int32x2_t(),
      set_int()));
  return 0;
}
