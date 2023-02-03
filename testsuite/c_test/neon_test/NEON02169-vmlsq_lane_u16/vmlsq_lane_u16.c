#include "neon.h"

int main() {
  print_uint16x8_t(
    vmlsq_lane_u16(
      set_uint16x8_t(),
      set_uint16x8_t(),
      set_uint16x4_t(),
      1));
  return 0;
}
