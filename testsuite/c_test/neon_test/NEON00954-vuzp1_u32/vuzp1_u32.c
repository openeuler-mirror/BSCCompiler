#include "neon.h"

int main() {
  print_uint32x2_t(
    vuzp1_u32(
      set_uint32x2_t(),
      set_uint32x2_t()));
  return 0;
}
