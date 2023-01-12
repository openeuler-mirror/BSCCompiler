#include "neon.h"

int main() {
  print_uint32x4_t(
    vmovl_high_u16(
      set_uint16x8_t()));
  return 0;
}
