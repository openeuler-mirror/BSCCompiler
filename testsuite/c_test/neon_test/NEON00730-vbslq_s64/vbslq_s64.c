#include "neon.h"

int main() {
  print_int64x2_t(
    vbslq_s64(
      set_uint64x2_t(),
      set_int64x2_t(),
      set_int64x2_t()));
  return 0;
}
