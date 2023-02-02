#include "neon.h"

int main() {
  uint8_t ptr[48] = { 0 };
  vst3q_lane_u8(
      ptr,
      set_uint8x16x3_t(),
      1);
  print_uint8_t_ptr(ptr, 48);
  return 0;
}
