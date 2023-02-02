#include "neon.h"

int main() {
  int16_t ptr[24] = { 0 };
  vst3q_s16(
      ptr,
      set_int16x8x3_t());
  print_int16_t_ptr(ptr, 24);
  return 0;
}
