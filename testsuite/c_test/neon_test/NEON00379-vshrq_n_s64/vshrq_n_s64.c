#include "neon.h"

int main() {
  print_int64x2_t(
    vshrq_n_s64(
      set_int64x2_t(),
      set_int_1()));
  return 0;
}
