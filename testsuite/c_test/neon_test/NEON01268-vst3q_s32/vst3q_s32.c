#include "neon.h"

int main() {
  int32_t ptr[12] = { 0 };
  vst3q_s32(
      ptr,
      set_int32x4x3_t());
  print_int32_t_ptr(ptr, 12);
  return 0;
}
