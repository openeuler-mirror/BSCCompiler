#include "neon.h"

int main() {
  print_uint64x1_t(
    vclt_s64(
      set_int64x1_t(),
      set_int64x1_t()));
  return 0;
}
