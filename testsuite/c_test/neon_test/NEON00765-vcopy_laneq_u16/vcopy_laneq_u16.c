#include "neon.h"

int main() {
  print_uint16x4_t(
    vcopy_laneq_u16(
      set_uint16x4_t(),
      set_int(),
      set_uint16x8_t(),
      set_int()));
  return 0;
}
