#include "neon.h"

int main() {
  int8_t ptr[16] = { 0 };
  vst1q_s8(
      ptr,
      set_int8x16_t());
  print_int8_t_ptr(ptr, 16);
  return 0;
}
