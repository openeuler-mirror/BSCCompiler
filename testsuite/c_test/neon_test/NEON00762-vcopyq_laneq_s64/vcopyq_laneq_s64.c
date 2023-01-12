#include "neon.h"

int main() {
  print_int64x2_t(
    vcopyq_laneq_s64(
      set_int64x2_t(),
      set_int(),
      set_int64x2_t(),
      set_int()));
  return 0;
}
