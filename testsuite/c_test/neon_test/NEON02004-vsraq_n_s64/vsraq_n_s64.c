#include "neon.h"

int main() {
  print_int64x2_t(
    vsraq_n_s64(
      set_int64x2_t(),
      set_int64x2_t(),
      1));
  return 0;
}
