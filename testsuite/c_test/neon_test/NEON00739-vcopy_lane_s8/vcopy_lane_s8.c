#include "neon.h"

int main() {
  print_int8x8_t(
    vcopy_lane_s8(
      set_int8x8_t(),
      set_int(),
      set_int8x8_t(),
      set_int()));
  return 0;
}
