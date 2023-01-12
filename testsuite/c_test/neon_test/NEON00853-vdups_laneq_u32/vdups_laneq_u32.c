#include "neon.h"

int main() {
  print_uint32_t(
    vdups_laneq_u32(
      set_uint32x4_t(),
      set_int()));
  return 0;
}
