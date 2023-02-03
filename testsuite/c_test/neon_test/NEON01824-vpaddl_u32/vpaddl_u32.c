#include "neon.h"

int main() {
  print_uint64x1_t(
    vpaddl_u32(
      set_uint32x2_t()));
  return 0;
}
