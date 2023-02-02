#include "neon.h"

int main() {
  print_uint16x8_t(
    vld1q_lane_u16(
      set_uint16_t_ptr(8),
      set_uint16x8_t(),
      1));
  return 0;
}
