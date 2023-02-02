#include "neon.h"

int main() {
  uint8_t ptr[64] = { 0 };
  vst4q_u8(
      ptr,
      set_uint8x16x4_t());
  print_uint8_t_ptr(ptr, 64);
  return 0;
}
