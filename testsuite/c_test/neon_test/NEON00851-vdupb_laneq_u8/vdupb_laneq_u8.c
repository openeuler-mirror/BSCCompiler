#include "neon.h"

int main() {
  print_uint8_t(
    vdupb_laneq_u8(
      set_uint8x16_t(),
      1));
  return 0;
}
