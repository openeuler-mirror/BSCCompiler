#include "neon.h"

int main() {
  print_int64x2x4_t(
    vld4q_lane_s64(
      set_int64_t_ptr(8),
      set_int64x2x4_t(),
      1));
  return 0;
}
