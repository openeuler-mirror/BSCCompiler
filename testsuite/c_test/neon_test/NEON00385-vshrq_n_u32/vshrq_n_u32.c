#include "neon.h"

int main() {
  print_uint32x4_t(
    vshrq_n_u32(
      set_uint32x4_t(),
      1));
  return 0;
}
