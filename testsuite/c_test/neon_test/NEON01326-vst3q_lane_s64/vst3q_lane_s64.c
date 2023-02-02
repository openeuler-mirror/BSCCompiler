#include "neon.h"

int main() {
  int64_t ptr[6] = { 0 };
  vst3q_lane_s64(
      ptr,
      set_int64x2x3_t(),
      1);
  print_int64_t_ptr(ptr, 6);
  return 0;
}
