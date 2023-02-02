#include "neon.h"

int main() {
  print_uint8x8x4_t(
    vld4_lane_u8(
      set_uint8_t_ptr(32),
      set_uint8x8x4_t(),
      1));
  return 0;
}
