#include "neon.h"

int main() {
  int64_t ptr[3] = { 0 };
  vst3_lane_s64(
      ptr,
      set_int64x1x3_t(),
      0);
  print_int64_t_ptr(ptr, 3);
  return 0;
}
