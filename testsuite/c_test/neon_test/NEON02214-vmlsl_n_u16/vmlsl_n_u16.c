#include "neon.h"

int main() {
  print_uint32x4_t(
    vmlsl_n_u16(
      set_uint32x4_t(),
      set_uint16x4_t(),
      set_uint16_t()));
  return 0;
}
