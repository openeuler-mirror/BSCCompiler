#include "neon.h"

int main() {
  print_int16x4_t(
    vld1_s16(
      set_int16_t_ptr(4)));
  return 0;
}
