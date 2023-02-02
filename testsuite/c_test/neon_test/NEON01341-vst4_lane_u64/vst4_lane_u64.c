#include "neon.h"

int main() {
  uint64_t ptr[4] = { 0 };
  vst4_lane_u64(
      ptr,
      set_uint64x1x4_t(),
      0);
  print_uint64_t_ptr(ptr, 4);
  return 0;
}
