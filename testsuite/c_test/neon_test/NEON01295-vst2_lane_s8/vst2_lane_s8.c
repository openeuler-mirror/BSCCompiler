#include "neon.h"

int main() {
  int8_t ptr[16] = { 0 };
  vst2_lane_s8(
      ptr,
      set_int8x8x2_t(),
      1);
  print_int8_t_ptr(ptr, 16);
  return 0;
}
