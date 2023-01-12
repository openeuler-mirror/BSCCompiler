#include "neon.h"

int main() {
  print_int8x16_t(
    vcopyq_lane_s8(
      set_int8x16_t(),
      set_int(),
      set_int8x8_t(),
      set_int()));
  return 0;
}
