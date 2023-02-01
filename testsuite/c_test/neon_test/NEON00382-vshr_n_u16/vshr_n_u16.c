#include "neon.h"

int main() {
  print_uint16x4_t(
    vshr_n_u16(
      set_uint16x4_t(),
      set_int_1()));
  return 0;
}
