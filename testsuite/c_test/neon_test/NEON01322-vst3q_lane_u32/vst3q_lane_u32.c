#include "neon.h"

int main() {
  uint32_t ptr[12] = { 0 };
  vst3q_lane_u32(
      ptr,
      set_uint32x4x3_t(),
      1);
  print_uint32_t_ptr(ptr, 12);
  return 0;
}
