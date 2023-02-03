#include "neon.h"

int main() {
  print_uint8x8_t(
    vget_low_u8(
      set_uint8x16_t()));
  return 0;
}
