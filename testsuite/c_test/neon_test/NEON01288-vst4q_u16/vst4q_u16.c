#include "neon.h"

int main() {
  uint16_t ptr[32] = { 0 };
  vst4q_u16(
      ptr,
      set_uint16x8x4_t());
  print_uint16_t_ptr(ptr, 32);
  return 0;
}
