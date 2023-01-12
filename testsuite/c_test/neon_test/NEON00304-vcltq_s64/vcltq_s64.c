#include "neon.h"

int main() {
  print_uint64x2_t(
    vcltq_s64(
      set_int64x2_t(),
      set_int64x2_t()));
  return 0;
}
