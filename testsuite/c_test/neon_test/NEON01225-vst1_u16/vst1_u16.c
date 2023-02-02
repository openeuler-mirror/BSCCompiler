#include "neon.h"

int main() {
  uint16_t ptr[4] = { 0 };
  vst1_u16(
      ptr,
      set_uint16x4_t());
  print_uint16_t_ptr(ptr, 4);
  return 0;
}
