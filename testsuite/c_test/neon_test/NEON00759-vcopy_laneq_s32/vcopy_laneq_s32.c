#include "neon.h"

int main() {
  print_int32x2_t(
    vcopy_laneq_s32(
      set_int32x2_t(),
      1,
      set_int32x4_t(),
      1));
  return 0;
}
