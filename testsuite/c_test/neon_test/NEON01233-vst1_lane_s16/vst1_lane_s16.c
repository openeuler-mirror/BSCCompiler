#include "neon.h"

int main() {
  int16_t ptr[4] = { 0 };
  vst1_lane_s16(
      ptr,
      set_int16x4_t(),
      1);
  print_int16_t_ptr(ptr, 4);
  return 0;
}
