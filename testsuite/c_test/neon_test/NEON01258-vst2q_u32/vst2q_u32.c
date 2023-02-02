#include "neon.h"

int main() {
  uint32_t ptr[8] = { 0 };
  vst2q_u32(
      ptr,
      set_uint32x4x2_t());
  print_uint32_t_ptr(ptr, 8);
  return 0;
}
