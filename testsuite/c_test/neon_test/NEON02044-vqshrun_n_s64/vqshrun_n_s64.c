#include "neon.h"

int main() {
  print_uint32x2_t(
    vqshrun_n_s64(
      set_int64x2_t(),
      1));
  return 0;
}
