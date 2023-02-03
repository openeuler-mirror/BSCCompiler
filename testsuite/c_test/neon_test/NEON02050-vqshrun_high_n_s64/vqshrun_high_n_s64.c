#include "neon.h"

int main() {
  print_uint32x4_t(
    vqshrun_high_n_s64(
      set_uint32x2_t(),
      set_int64x2_t(),
      1));
  return 0;
}
