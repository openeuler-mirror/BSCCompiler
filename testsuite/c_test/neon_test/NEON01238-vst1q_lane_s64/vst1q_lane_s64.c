#include "neon.h"

int main() {
  int64_t ptr[2] = { 0 };
  vst1q_lane_s64(
      ptr,
      set_int64x2_t(),
      1);
  print_int64_t_ptr(ptr, 2);
  return 0;
}
