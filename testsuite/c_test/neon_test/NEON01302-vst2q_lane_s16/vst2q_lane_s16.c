#include "neon.h"

int main() {
  int16_t ptr[16] = { 0 };
  vst2q_lane_s16(
      ptr,
      set_int16x8x2_t(),
      1);
  print_int16_t_ptr(ptr, 16);
  return 0;
}
