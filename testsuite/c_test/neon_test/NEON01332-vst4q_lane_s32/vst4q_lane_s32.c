#include "neon.h"

int main() {
  int32_t ptr[16] = { 0 };
  vst4q_lane_s32(
      ptr,
      set_int32x4x4_t(),
      1);
  print_int32_t_ptr(ptr, 16);
  return 0;
}
