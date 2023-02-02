#include "neon.h"

int main() {
  uint16_t ptr[12] = { 0 };
  vst3_u16(
      ptr,
      set_uint16x4x3_t());
  print_uint16_t_ptr(ptr, 12);
  return 0;
}
