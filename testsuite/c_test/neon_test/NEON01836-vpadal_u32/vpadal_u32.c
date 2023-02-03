#include "neon.h"

int main() {
  print_uint64x1_t(
    vpadal_u32(
      set_uint64x1_t(),
      set_uint32x2_t()));
  return 0;
}
