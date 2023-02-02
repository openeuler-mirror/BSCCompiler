#include "neon.h"

int main() {
  int8_t ptr[32] = { 0 };
  vst2q_lane_s8(
      ptr,
      set_int8x16x2_t(),
      1);
  print_int8_t_ptr(ptr, 32);
  return 0;
}
