#include "neon.h"

int main() {
  print_uint32x2_t(
    vmls_n_u32(
      set_uint32x2_t(),
      set_uint32x2_t(),
      set_uint32_t()));
  return 0;
}
