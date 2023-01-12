#include "neon.h"

int main() {
  print_int32x4_t(
    vcopyq_lane_s32(
      set_int32x4_t(),
      set_int(),
      set_int32x2_t(),
      set_int()));
  return 0;
}
