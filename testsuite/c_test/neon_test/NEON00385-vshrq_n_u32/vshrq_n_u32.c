#include "neon.h"

int main() {
  print_uint32x4_t(
    vshrq_n_u32(
      set_uint32x4_t(),
      set_int()));
  return 0;
}
