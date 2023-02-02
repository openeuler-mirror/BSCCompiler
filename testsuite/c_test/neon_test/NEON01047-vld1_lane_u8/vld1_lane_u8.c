#include "neon.h"

int main() {
  print_uint8x8_t(
    vld1_lane_u8(
      set_uint8_t_ptr(8),
      set_uint8x8_t(),
      1));
  return 0;
}
