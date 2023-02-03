#include "neon.h"

int main() {
  print_uint32x2_t(
    vmla_laneq_u32(
      set_uint32x2_t(),
      set_uint32x2_t(),
      set_uint32x4_t(),
      1));
  return 0;
}
