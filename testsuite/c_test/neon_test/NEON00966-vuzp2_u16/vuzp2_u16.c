#include "neon.h"

int main() {
  print_uint16x4_t(
    vuzp2_u16(
      set_uint16x4_t(),
      set_uint16x4_t()));
  return 0;
}