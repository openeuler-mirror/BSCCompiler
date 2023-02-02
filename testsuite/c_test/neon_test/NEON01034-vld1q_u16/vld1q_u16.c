#include "neon.h"

int main() {
  print_uint16x8_t(
    vld1q_u16(
      set_uint16_t_ptr(8)));
  return 0;
}
