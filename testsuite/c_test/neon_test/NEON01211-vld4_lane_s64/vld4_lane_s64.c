#include "neon.h"

int main() {
  print_int64x1x4_t(
    vld4_lane_s64(
      set_int64_t_ptr(4),
      set_int64x1x4_t(),
      0));
  return 0;
}
