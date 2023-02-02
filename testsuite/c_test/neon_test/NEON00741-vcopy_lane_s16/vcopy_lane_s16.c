#include "neon.h"

int main() {
  print_int16x4_t(
    vcopy_lane_s16(
      set_int16x4_t(),
      1,
      set_int16x4_t(),
      1));
  return 0;
}
