#include "neon.h"

int main() {
  print_int64x1_t(
    vset_lane_s64(
      set_int64_t(),
      set_int64x1_t(),
      0));
  return 0;
}
