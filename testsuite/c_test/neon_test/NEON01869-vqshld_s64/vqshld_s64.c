#include "neon.h"

int main() {
  print_int64_t(
    vqshld_s64(
      set_int64_t(),
      set_int64_t()));
  return 0;
}
