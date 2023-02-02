#include "neon.h"

int main() {
  uint32_t ptr[8] = { 0 };
  vst4_lane_u32(
      ptr,
      set_uint32x2x4_t(),
      1);
  print_uint32_t_ptr(ptr, 8);
  return 0;
}
