#include "neon.h"

int main() {
  uint8_t ptr[24] = { 0 };
  vst3_lane_u8(
      ptr,
      set_uint8x8x3_t(),
      1);
  print_uint8_t_ptr(ptr, 24);
  return 0;
}
