#include "neon.h"

int main() {
  uint16_t ptr[8] = { 0 };
  vst1q_u16(
      ptr,
      set_uint16x8_t());
  print_uint16_t_ptr(ptr, 8);
  return 0;
}
