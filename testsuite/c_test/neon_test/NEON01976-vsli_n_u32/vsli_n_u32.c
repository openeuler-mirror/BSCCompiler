#include "neon.h"

int main() {
  print_uint32x2_t(
    vsli_n_u32(
      set_uint32x2_t(),
      set_uint32x2_t(),
      1));
  return 0;
}
