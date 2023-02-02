#include "neon.h"

int main() {
  uint32_t ptr[6] = { 0 };
  vst3_lane_u32(
      ptr,
      set_uint32x2x3_t(),
      1);
  print_uint32_t_ptr(ptr, 6);
  return 0;
}
