#include "neon.h"

int main() {
  print_int64x1_t(
    vsli_n_s64(
      set_int64x1_t(),
      set_int64x1_t(),
      0));
  return 0;
}
