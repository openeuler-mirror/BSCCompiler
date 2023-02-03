#include "neon.h"

int main() {
  print_uint64x1_t(
    vrshl_u64(
      set_uint64x1_t(),
      set_int64x1_t()));
  return 0;
}
