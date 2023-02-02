#include "neon.h"

int main() {
  print_int64x1_t(
    vcopy_lane_s64(
      set_int64x1_t(),
      0,
      set_int64x1_t(),
      0));
  return 0;
}
