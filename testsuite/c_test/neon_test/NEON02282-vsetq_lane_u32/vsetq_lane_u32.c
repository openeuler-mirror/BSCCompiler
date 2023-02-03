#include "neon.h"

int main() {
  print_uint32x4_t(
    vsetq_lane_u32(
      set_uint32_t(),
      set_uint32x4_t(),
      1));
  return 0;
}
