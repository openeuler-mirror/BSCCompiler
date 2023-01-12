#include "neon.h"

int main() {
  print_int64x2_t(
    vcombine_s64(
      set_int64x1_t(),
      set_int64x1_t()));
  return 0;
}
