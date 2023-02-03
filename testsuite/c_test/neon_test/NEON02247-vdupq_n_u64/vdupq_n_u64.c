#include "neon.h"

int main() {
  print_uint64x2_t(
    vdupq_n_u64(
      set_uint64_t()));
  return 0;
}
