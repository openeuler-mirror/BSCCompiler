#include "neon.h"

int main() {
  int8_t ptr[8] = { 0 };
  vst1_s8(
      ptr,
      set_int8x8_t());
  print_int8_t_ptr(ptr, 8);
  return 0;
}
