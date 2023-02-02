#include "neon.h"

int main() {
  print_uint16x8x3_t(
    vld3q_u16(
      set_uint16_t_ptr(24)));
  return 0;
}
