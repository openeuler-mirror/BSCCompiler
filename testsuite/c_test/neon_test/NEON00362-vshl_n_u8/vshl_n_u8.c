#include "neon.h"

int main() {
  print_uint8x8_t(
    vshl_n_u8(
      set_uint8x8_t(),
      set_int()));
  return 0;
}
