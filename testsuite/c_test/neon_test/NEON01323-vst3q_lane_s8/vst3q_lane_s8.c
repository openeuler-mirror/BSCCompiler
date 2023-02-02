#include "neon.h"

int main() {
  int8_t ptr[48] = { 0 };
  vst3q_lane_s8(
      ptr,
      set_int8x16x3_t(),
      1);
  print_int8_t_ptr(ptr, 48);
  return 0;
}
