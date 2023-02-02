#include "neon.h"

int main() {
  int8_t ptr[32] = { 0 };
  vst4_lane_s8(
      ptr,
      set_int8x8x4_t(),
      1);
  print_int8_t_ptr(ptr, 32);
  return 0;
}
