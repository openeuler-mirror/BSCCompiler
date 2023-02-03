#include "neon.h"

int main() {
  print_int64x2_t(
    vdupq_n_s64(
      set_int64_t()));
  return 0;
}
