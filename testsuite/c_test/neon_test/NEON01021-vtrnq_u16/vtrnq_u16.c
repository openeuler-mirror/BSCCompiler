#include "neon.h"

int main() {
  print_uint16x8x2_t(
    vtrnq_u16(
      set_uint16x8_t(),
      set_uint16x8_t()));
  return 0;
}
