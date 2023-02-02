#include "neon.h"

int main() {
  int32_t ptr[6] = { 0 };
  vst3_s32(
      ptr,
      set_int32x2x3_t());
  print_int32_t_ptr(ptr, 6);
  return 0;
}
