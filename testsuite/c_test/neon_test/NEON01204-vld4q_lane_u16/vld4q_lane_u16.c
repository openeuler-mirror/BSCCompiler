#include "neon.h"

int main() {
  print_uint16x8x4_t(
    vld4q_lane_u16(
      set_uint16_t_ptr(32),
      set_uint16x8x4_t(),
      1));
  return 0;
}
