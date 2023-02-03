#include "neon.h"

int main() {
  print_uint16x4_t(
    vmla_lane_u16(
      set_uint16x4_t(),
      set_uint16x4_t(),
      set_uint16x4_t(),
      1));
  return 0;
}
