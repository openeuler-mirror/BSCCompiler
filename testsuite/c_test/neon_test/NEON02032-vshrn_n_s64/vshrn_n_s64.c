#include "neon.h"

int main() {
  print_int32x2_t(
    vshrn_n_s64(
      set_int64x2_t(),
      1));
  return 0;
}
