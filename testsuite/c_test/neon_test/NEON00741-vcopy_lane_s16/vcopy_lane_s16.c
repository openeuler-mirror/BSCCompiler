#include "neon.h"

int main() {
  print_int16x4_t(
    vcopy_lane_s16(
      set_int16x4_t(),
      set_int(),
      set_int16x4_t(),
      set_int()));
  return 0;
}
