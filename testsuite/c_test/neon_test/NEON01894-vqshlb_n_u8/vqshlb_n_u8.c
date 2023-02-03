#include "neon.h"

int main() {
  print_uint8_t(
    vqshlb_n_u8(
      set_uint8_t(),
      1));
  return 0;
}
