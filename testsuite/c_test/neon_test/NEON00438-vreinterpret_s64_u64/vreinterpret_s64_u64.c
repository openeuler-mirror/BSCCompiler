#include "neon.h"

int main() {
  print_int64x1_t(
    vreinterpret_s64_u64(
      set_uint64x1_t()));
  return 0;
}
