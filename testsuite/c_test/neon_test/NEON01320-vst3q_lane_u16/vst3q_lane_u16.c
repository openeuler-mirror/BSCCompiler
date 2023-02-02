#include "neon.h"

int main() {
  uint16_t ptr[24] = { 0 };
  vst3q_lane_u16(
      ptr,
      set_uint16x8x3_t(),
      1);
  print_uint16_t_ptr(ptr, 24);
  return 0;
}
