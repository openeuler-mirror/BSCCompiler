#include "neon.h"

int main() {
  print_int16x4x3_t(
    vld3_s16(
      set_int16_t_ptr(12)));
  return 0;
}
