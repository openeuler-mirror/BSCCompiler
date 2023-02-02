#include "neon.h"

int main() {
  uint8_t ptr[16] = { 0 };
  vst2_lane_u8(
      ptr,
      set_uint8x8x2_t(),
      1);
  print_uint8_t_ptr(ptr, 16);
  return 0;
}
