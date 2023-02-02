#include "neon.h"

int main() {
  print_int64x2_t(
    vcopyq_laneq_s64(
      set_int64x2_t(),
      1,
      set_int64x2_t(),
      1));
  return 0;
}
