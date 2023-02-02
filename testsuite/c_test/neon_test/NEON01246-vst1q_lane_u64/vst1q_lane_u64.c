#include "neon.h"

int main() {
  uint64_t ptr[2] = { 0 };
  vst1q_lane_u64(
      ptr,
      set_uint64x2_t(),
      1);
  print_uint64_t_ptr(ptr, 2);
  return 0;
}
