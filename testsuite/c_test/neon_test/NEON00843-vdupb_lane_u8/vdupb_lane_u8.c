#include "neon.h"

int main() {
  print_uint8_t(
    vdupb_lane_u8(
      set_uint8x8_t(),
      set_int()));
  return 0;
}
