#include "neon.h"

int main() {
  uint8_t ptr[32] = { 0 };
  vst4_u8(
      ptr,
      set_uint8x8x4_t());
  print_uint8_t_ptr(ptr, 32);
  return 0;
}
