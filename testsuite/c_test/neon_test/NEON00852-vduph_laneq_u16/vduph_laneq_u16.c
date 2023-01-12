#include "neon.h"

int main() {
  print_uint16_t(
    vduph_laneq_u16(
      set_uint16x8_t(),
      set_int()));
  return 0;
}
