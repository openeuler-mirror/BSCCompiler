#include "neon.h"

int main() {
  print_uint64x1_t(
    vqshlu_n_s64(
      set_int64x1_t(),
      0));
  return 0;
}
