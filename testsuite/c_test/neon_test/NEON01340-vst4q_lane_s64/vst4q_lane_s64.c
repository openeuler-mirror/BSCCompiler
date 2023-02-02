#include "neon.h"

int main() {
  int64_t ptr[8] = { 0 };
  vst4q_lane_s64(
      ptr,
      set_int64x2x4_t(),
      1);
  print_int64_t_ptr(ptr, 8);
  return 0;
}
