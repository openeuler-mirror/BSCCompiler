#include "neon.h"

int main() {
  print_int16_t(
    vqrdmulhh_lane_s16(
      set_int16_t(),
      set_int16x4_t(),
      1));
  return 0;
}
