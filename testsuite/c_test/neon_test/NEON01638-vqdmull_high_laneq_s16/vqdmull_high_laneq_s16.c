#include "neon.h"

int main() {
  print_int32x4_t(
    vqdmull_high_laneq_s16(
      set_int16x8_t(),
      set_int16x8_t(),
      1));
  return 0;
}
