#include "neon.h"

int main() {
  print_uint16x4_t(
    vshl_n_u16(
      set_uint16x4_t(),
      set_int()));
  return 0;
}
