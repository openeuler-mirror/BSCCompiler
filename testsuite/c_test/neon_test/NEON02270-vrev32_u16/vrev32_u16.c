#include "neon.h"

int main() {
  print_uint16x4_t(
    vrev32_u16(
      set_uint16x4_t()));
  return 0;
}
