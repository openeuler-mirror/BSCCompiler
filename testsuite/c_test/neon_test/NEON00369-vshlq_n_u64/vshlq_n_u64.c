#include "neon.h"

int main() {
  print_uint64x2_t(
    vshlq_n_u64(
      set_uint64x2_t(),
      set_int()));
  return 0;
}
