#include "neon.h"

int main() {
  uint64_t ptr[1] = { 0 };
  vst1_lane_u64(
      ptr,
      set_uint64x1_t(),
      0);
  print_uint64_t_ptr(ptr, 1);
  return 0;
}
