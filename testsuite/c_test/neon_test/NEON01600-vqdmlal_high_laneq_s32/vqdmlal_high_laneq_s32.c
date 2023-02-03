#include "neon.h"

int main() {
  print_int64x2_t(
    vqdmlal_high_laneq_s32(
      set_int64x2_t(),
      set_int32x4_t(),
      set_int32x4_t(),
      1));
  return 0;
}
