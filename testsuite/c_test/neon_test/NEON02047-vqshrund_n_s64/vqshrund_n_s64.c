#include "neon.h"

int main() {
  print_uint32_t(
    vqshrund_n_s64(
      set_int64_t(),
      1));
  return 0;
}
