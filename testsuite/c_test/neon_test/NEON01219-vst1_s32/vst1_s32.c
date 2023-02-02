#include "neon.h"

int main() {
  int32_t ptr[2] = { 0 };
  vst1_s32(
      ptr,
      set_int32x2_t());
  print_int32_t_ptr(ptr, 2);
  return 0;
}
