#include "neon.h"

int main() {
  int16_t ptr[12] = { 0 };
  vst3_lane_s16(
      ptr,
      set_int16x4x3_t(),
      1);
  print_int16_t_ptr(ptr, 12);
  return 0;
}
