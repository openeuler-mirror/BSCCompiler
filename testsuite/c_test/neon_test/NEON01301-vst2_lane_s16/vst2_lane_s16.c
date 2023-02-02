#include "neon.h"

int main() {
  int16_t ptr[8] = { 0 };
  vst2_lane_s16(
      ptr,
      set_int16x4x2_t(),
      1);
  print_int16_t_ptr(ptr, 8);
  return 0;
}
