#include "neon.h"

int main() {
  print_uint32x4_t(
    vcopyq_lane_u32(
      set_uint32x4_t(),
      set_int(),
      set_uint32x2_t(),
      set_int()));
  return 0;
}
