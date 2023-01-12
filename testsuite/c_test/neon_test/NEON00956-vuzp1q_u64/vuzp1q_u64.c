#include "neon.h"

int main() {
  print_uint64x2_t(
    vuzp1q_u64(
      set_uint64x2_t(),
      set_uint64x2_t()));
  return 0;
}
