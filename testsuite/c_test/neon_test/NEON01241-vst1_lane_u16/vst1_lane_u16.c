#include "neon.h"

int main() {
  uint16_t ptr[4] = { 0 };
  vst1_lane_u16(
      ptr,
      set_uint16x4_t(),
      1);
  print_uint16_t_ptr(ptr, 4);
  return 0;
}
