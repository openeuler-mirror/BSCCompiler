#include "neon.h"

int main() {
  uint32_t ptr[16] = { 0 };
  vst4q_lane_u32(
      ptr,
      set_uint32x4x4_t(),
      1);
  print_uint32_t_ptr(ptr, 16);
  return 0;
}
