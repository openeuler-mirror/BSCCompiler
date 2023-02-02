#include "neon.h"

int main() {
  print_int16x8_t(
    vdupq_lane_s16(
      set_int16x4_t(),
      1));
  return 0;
}
