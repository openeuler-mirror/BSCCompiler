#include "neon.h"

int main() {
  uint16_t ptr[12] = { 0 };
  vst3_lane_u16(
      ptr,
      set_uint16x4x3_t(),
      1);
  print_uint16_t_ptr(ptr, 12);
  return 0;
}
