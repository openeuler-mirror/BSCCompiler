#include "neon.h"

int main() {
  uint16_t ptr[24] = { 0 };
  vst3q_u16(
      ptr,
      set_uint16x8x3_t());
  print_uint16_t_ptr(ptr, 24);
  return 0;
}
