#include "neon.h"

int main() {
  print_int16x4x2_t(
    vld2_s16(
      set_int16_t_ptr(8)));
  return 0;
}
