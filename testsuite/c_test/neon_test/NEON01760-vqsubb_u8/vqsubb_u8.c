#include "neon.h"

int main() {
  print_uint8_t(
    vqsubb_u8(
      set_uint8_t(),
      set_uint8_t()));
  return 0;
}
