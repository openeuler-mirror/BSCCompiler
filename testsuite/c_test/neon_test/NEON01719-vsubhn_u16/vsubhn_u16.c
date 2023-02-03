#include "neon.h"

int main() {
  print_uint8x8_t(
    vsubhn_u16(
      set_uint16x8_t(),
      set_uint16x8_t()));
  return 0;
}
