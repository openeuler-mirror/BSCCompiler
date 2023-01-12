#include "neon.h"

int main() {
  print_uint16x4_t(
    vdup_laneq_u16(
      set_uint16x8_t(),
      set_int()));
  return 0;
}
