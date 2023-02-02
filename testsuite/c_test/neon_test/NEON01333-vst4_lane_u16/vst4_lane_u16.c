#include "neon.h"

int main() {
  uint16_t ptr[16] = { 0 };
  vst4_lane_u16(
      ptr,
      set_uint16x4x4_t(),
      1);
  print_uint16_t_ptr(ptr, 16);
  return 0;
}
