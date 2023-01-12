#include "neon.h"

int main() {
  print_uint64x2_t(
    vzip2q_u64(
      set_uint64x2_t(),
      set_uint64x2_t()));
  return 0;
}
