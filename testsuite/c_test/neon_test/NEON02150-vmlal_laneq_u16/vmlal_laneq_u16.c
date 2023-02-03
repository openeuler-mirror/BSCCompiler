#include "neon.h"

int main() {
  print_uint32x4_t(
    vmlal_laneq_u16(
      set_uint32x4_t(),
      set_uint16x4_t(),
      set_uint16x8_t(),
      1));
  return 0;
}
