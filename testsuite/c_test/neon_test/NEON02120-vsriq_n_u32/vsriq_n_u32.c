#include "neon.h"

int main() {
  print_uint32x4_t(
    vsriq_n_u32(
      set_uint32x4_t(),
      set_uint32x4_t(),
      1));
  return 0;
}
