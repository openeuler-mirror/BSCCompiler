#include "neon.h"

int main() {
  print_int64_t(
    vget_lane_s64(
      set_int64x1_t(),
      0));
  return 0;
}
