#include "neon.h"

int main() {
  print_uint32x2_t(
    vdup_lane_u32(
      set_uint32x2_t(),
      set_int()));
  return 0;
}
