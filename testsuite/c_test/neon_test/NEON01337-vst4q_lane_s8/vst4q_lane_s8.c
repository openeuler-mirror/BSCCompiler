#include "neon.h"

int main() {
  int8_t ptr[64] = { 0 };
  vst4q_lane_s8(
      ptr,
      set_int8x16x4_t(),
      1);
  print_int8_t_ptr(ptr, 64);
  return 0;
}
