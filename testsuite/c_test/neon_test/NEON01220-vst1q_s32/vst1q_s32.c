#include "neon.h"

int main() {
  int32_t ptr[4] = { 0 };
  vst1q_s32(
      ptr,
      set_int32x4_t());
  print_int32_t_ptr(ptr, 4);
  return 0;
}
