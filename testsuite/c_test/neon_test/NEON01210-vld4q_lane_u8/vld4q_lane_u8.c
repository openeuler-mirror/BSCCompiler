#include "neon.h"

int main() {
  print_uint8x16x4_t(
    vld4q_lane_u8(
      set_uint8_t_ptr(64),
      set_uint8x16x4_t(),
      1));
  return 0;
}
