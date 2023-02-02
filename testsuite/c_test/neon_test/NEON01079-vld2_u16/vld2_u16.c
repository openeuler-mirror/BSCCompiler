#include "neon.h"

int main() {
  print_uint16x4x2_t(
    vld2_u16(
      set_uint16_t_ptr(8)));
  return 0;
}
