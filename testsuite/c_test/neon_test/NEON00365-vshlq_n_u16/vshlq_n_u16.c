#include "neon.h"

int main() {
  print_uint16x8_t(
    vshlq_n_u16(
      set_uint16x8_t(),
      1));
  return 0;
}
