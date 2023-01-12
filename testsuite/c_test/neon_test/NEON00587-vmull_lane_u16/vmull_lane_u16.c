#include "neon.h"

int main() {
  print_uint32x4_t(
    vmull_lane_u16(
      set_uint16x4_t(),
      set_uint16x4_t(),
      set_int()));
  return 0;
}
