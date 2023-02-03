#include "neon.h"

int main() {
  print_int32x4_t(
    vaddhn_high_s64(
      set_int32x2_t(),
      set_int64x2_t(),
      set_int64x2_t()));
  return 0;
}
