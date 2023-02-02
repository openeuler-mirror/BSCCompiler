#include "neon.h"

int main() {
  int32_t ptr[4] = { 0 };
  vst1q_lane_s32(
      ptr,
      set_int32x4_t(),
      1);
  print_int32_t_ptr(ptr, 4);
  return 0;
}
