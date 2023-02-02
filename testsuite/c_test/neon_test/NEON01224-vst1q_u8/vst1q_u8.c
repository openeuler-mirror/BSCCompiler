#include "neon.h"

int main() {
  uint8_t ptr[16] = { 0 };
  vst1q_u8(
      ptr,
      set_uint8x16_t());
  print_uint8_t_ptr(ptr, 16);
  return 0;
}
