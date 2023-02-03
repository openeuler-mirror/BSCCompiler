#include "neon.h"

int main() {
  print_uint64x1_t(
    vget_low_u64(
      set_uint64x2_t()));
  return 0;
}
