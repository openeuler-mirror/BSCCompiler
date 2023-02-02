#include "neon.h"

int main() {
  uint32_t ptr[4] = { 0 };
  vst1q_lane_u32(
      ptr,
      set_uint32x4_t(),
      1);
  print_uint32_t_ptr(ptr, 4);
  return 0;
}
