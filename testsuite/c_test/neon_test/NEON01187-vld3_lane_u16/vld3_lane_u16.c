#include "neon.h"

int main() {
  print_uint16x4x3_t(
    vld3_lane_u16(
      set_uint16_t_ptr(12),
      set_uint16x4x3_t(),
      1));
  return 0;
}
