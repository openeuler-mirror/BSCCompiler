#include "neon.h"

int main() {
  print_int16x4_t(
    vmul_lane_s16(
      set_int16x4_t(),
      set_int16x4_t(),
      set_int()));
  return 0;
}
