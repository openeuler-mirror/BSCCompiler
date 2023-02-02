#include "neon.h"

int main() {
  print_uint8x16x3_t(
    vld3q_u8(
      set_uint8_t_ptr(48)));
  return 0;
}
