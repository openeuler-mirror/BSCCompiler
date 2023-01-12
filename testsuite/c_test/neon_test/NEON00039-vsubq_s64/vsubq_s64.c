#include "neon.h"

int main() {
  print_int64x2_t(
    vsubq_s64(
      set_int64x2_t(),
      set_int64x2_t()));
  return 0;
}
