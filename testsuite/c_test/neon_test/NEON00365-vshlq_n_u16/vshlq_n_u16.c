#include "neon.h"

int main() {
  print_uint16x8_t(
    vshlq_n_u16(
      set_uint16x8_t(),
      set_int()));
  return 0;
}
