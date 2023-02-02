#include "neon.h"

int main() {
  print_int16x8x3_t(
    vld3q_s16(
      set_int16_t_ptr(24)));
  return 0;
}
