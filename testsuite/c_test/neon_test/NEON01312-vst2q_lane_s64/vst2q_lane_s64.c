#include "neon.h"

int main() {
  int64_t ptr[4] = { 0 };
  vst2q_lane_s64(
      ptr,
      set_int64x2x2_t(),
      1);
  print_int64_t_ptr(ptr, 4);
  return 0;
}
