#include "neon.h"

int main() {
  print_uint64x1_t(
    vcopy_laneq_u64(
      set_uint64x1_t(),
      set_int(),
      set_uint64x2_t(),
      set_int()));
  return 0;
}
