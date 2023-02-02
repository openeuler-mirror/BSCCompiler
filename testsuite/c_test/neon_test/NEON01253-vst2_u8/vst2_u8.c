#include "neon.h"

int main() {
  uint8_t ptr[16] = { 0 };
  vst2_u8(
      ptr,
      set_uint8x8x2_t());
  print_uint8_t_ptr(ptr, 16);
  return 0;
}
