#include "neon.h"

int main() {
  print_uint16_t(
    vget_lane_u16(
      set_uint16x4_t(),
      1));
  return 0;
}
