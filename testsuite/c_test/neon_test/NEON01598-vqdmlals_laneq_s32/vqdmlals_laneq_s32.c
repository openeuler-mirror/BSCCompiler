#include "neon.h"

int main() {
  print_int64_t(
    vqdmlals_laneq_s32(
      set_int64_t(),
      set_int32_t(),
      set_int32x4_t(),
      1));
  return 0;
}
