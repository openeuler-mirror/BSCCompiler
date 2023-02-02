#include "neon.h"

int main() {
  uint16_t ptr[32] = { 0 };
  vst4q_lane_u16(
      ptr,
      set_uint16x8x4_t(),
      1);
  print_uint16_t_ptr(ptr, 32);
  return 0;
}
