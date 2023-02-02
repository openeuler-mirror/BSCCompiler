#include "neon.h"

int main() {
  uint8_t ptr[32] = { 0 };
  vst2q_u8(
      ptr,
      set_uint8x16x2_t());
  print_uint8_t_ptr(ptr, 32);
  return 0;
}
