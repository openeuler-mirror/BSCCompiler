#include "neon.h"

int main() {
  print_int64_t(
    vqshld_n_s64(
      set_int64_t(),
      1));
  return 0;
}
