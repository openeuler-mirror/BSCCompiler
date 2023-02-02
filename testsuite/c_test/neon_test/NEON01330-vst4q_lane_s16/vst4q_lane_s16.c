#include "neon.h"

int main() {
  int16_t ptr[32] = { 0 };
  vst4q_lane_s16(
      ptr,
      set_int16x8x4_t(),
      1);
  print_int16_t_ptr(ptr, 32);
  return 0;
}
