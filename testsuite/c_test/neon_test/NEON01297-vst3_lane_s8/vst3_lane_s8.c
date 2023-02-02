#include "neon.h"

int main() {
  int8_t ptr[24] = { 0 };
  vst3_lane_s8(
      ptr,
      set_int8x8x3_t(),
      1);
  print_int8_t_ptr(ptr, 24);
  return 0;
}
