#include "neon.h"

int main() {
  print_int64x1_t(
    vshr_n_s64(
      set_int64x1_t(),
      set_int()));
  return 0;
}
