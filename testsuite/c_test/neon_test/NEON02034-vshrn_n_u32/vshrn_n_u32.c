#include "neon.h"

int main() {
  print_uint16x4_t(
    vshrn_n_u32(
      set_uint32x4_t(),
      1));
  return 0;
}
