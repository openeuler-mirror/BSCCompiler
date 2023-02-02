#include "neon.h"

int main() {
  uint32_t ptr[2] = { 0 };
  vst1_u32(
      ptr,
      set_uint32x2_t());
  print_uint32_t_ptr(ptr, 2);
  return 0;
}
