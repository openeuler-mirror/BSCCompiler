#include "neon.h"

int main() {
  print_uint64x1_t(
    vext_u64(
      set_uint64x1_t(),
      set_uint64x1_t(),
      set_int()));
  return 0;
}
