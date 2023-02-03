#include "neon.h"

int main() {
  print_uint16x8_t(
    vmlaq_laneq_u16(
      set_uint16x8_t(),
      set_uint16x8_t(),
      set_uint16x8_t(),
      1));
  return 0;
}
