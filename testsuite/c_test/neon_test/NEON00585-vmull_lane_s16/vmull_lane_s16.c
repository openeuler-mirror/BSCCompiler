#include "neon.h"

int main() {
  print_int32x4_t(
    vmull_lane_s16(
      set_int16x4_t(),
      set_int16x4_t(),
      1));
  return 0;
}
