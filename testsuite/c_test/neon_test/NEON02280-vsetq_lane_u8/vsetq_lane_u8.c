#include "neon.h"

int main() {
  print_uint8x16_t(
    vsetq_lane_u8(
      set_uint8_t(),
      set_uint8x16_t(),
      1));
  return 0;
}
