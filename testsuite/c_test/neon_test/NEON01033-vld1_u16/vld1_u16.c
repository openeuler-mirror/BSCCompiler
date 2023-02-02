#include "neon.h"

int main() {
  print_uint16x4_t(
    vld1_u16(
      set_uint16_t_ptr(4)));
  return 0;
}
