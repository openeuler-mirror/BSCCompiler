#include "neon.h"

int main() {
  int32_t ptr[8] = { 0 };
  vst4_lane_s32(
      ptr,
      set_int32x2x4_t(),
      1);
  print_int32_t_ptr(ptr, 8);
  return 0;
}
