#include "neon.h"

int main() {
  print_uint16x4_t(
    vmla_n_u16(
      set_uint16x4_t(),
      set_uint16x4_t(),
      set_uint16_t()));
  return 0;
}
