#include "neon.h"

int main() {
  print_int8_t(
    vdupb_lane_s8(
      set_int8x8_t(),
      1));
  return 0;
}
