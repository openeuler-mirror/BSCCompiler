#include "neon.h"

int main() {
  int16_t ptr[16] = { 0 };
  vst4_s16(
      ptr,
      set_int16x4x4_t());
  print_int16_t_ptr(ptr, 16);
  return 0;
}
