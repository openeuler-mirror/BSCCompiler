#include "neon.h"

int main() {
  print_int32x4_t(
    vmull_high_laneq_s16(
      set_int16x8_t(),
      set_int16x8_t(),
      set_int()));
  return 0;
}
