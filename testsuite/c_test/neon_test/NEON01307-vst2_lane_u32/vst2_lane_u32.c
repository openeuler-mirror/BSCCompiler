#include "neon.h"

int main() {
  uint32_t ptr[4] = { 0 };
  vst2_lane_u32(
      ptr,
      set_uint32x2x2_t(),
      1);
  print_uint32_t_ptr(ptr, 4);
  return 0;
}
