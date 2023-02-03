#include "neon.h"

int main() {
  print_int32_t(
    vqshrnd_n_s64(
      set_int64_t(),
      1));
  return 0;
}
