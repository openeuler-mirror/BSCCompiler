#include "neon.h"

int main() {
  print_uint64x2_t(
    vextq_u64(
      set_uint64x2_t(),
      set_uint64x2_t(),
      1));
  return 0;
}
