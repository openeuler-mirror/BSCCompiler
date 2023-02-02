#include "neon.h"

int main() {
  int16_t ptr[8] = { 0 };
  vst1q_s16(
      ptr,
      set_int16x8_t());
  print_int16_t_ptr(ptr, 8);
  return 0;
}
