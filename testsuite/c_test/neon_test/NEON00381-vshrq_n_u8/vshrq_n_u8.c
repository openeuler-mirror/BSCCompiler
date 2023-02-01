#include "neon.h"

int main() {
  print_uint8x16_t(
    vshrq_n_u8(
      set_uint8x16_t(),
      set_int_1()));
  return 0;
}
