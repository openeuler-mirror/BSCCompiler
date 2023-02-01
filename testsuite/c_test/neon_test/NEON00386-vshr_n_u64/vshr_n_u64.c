#include "neon.h"

int main() {
  print_uint64x1_t(
    vshr_n_u64(
      set_uint64x1_t(),
      set_int_1()));
  return 0;
}
