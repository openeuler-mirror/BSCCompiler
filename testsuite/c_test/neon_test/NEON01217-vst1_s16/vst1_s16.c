#include "neon.h"

int main() {
  int16_t ptr[4] = { 0 };
  vst1_s16(
      ptr,
      set_int16x4_t());
  print_int16_t_ptr(ptr, 4);
  return 0;
}
