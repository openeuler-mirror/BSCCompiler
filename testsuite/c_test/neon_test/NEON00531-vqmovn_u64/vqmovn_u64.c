#include "neon.h"

int main() {
  print_uint32x2_t(
    vqmovn_u64(
      set_uint64x2_t()));
  return 0;
}
