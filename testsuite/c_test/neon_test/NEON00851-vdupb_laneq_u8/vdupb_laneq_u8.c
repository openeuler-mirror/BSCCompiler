#include "neon.h"

int main() {
  print_uint8_t(
    vdupb_laneq_u8(
      set_uint8x16_t(),
      set_int()));
  return 0;
}
