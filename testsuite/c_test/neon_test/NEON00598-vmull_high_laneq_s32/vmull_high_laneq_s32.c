#include "neon.h"

int main() {
  print_int64x2_t(
    vmull_high_laneq_s32(
      set_int32x4_t(),
      set_int32x4_t(),
      set_int()));
  return 0;
}
