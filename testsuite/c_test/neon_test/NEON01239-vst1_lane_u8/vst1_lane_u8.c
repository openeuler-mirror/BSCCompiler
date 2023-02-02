#include "neon.h"

int main() {
  uint8_t ptr[8] = { 0 };
  vst1_lane_u8(
      ptr,
      set_uint8x8_t(),
      1);
  print_uint8_t_ptr(ptr, 8);
  return 0;
}
