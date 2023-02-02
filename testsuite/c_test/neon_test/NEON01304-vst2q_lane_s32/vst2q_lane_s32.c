#include "neon.h"

int main() {
  int32_t ptr[8] = { 0 };
  vst2q_lane_s32(
      ptr,
      set_int32x4x2_t(),
      1);
  print_int32_t_ptr(ptr, 8);
  return 0;
}
