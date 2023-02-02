#include "neon.h"

int main() {
  uint16_t ptr[16] = { 0 };
  vst4_u16(
      ptr,
      set_uint16x4x4_t());
  print_uint16_t_ptr(ptr, 16);
  return 0;
}
