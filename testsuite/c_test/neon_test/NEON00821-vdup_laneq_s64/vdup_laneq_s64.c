#include "neon.h"

int main() {
  print_int64x1_t(
    vdup_laneq_s64(
      set_int64x2_t(),
      0));
  return 0;
}
