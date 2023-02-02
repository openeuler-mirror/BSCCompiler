#include "neon.h"

int main() {
  print_int32x2_t(
    vmul_laneq_s32(
      set_int32x2_t(),
      set_int32x4_t(),
      1));
  return 0;
}
