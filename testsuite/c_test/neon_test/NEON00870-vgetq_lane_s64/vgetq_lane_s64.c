#include "neon.h"

int main() {
  print_int64_t(
    vgetq_lane_s64(
      set_int64x2_t(),
      1));
  return 0;
}
