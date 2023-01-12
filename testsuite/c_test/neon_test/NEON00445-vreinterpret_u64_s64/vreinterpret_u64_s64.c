#include "neon.h"

int main() {
  print_uint64x1_t(
    vreinterpret_u64_s64(
      set_int64x1_t()));
  return 0;
}
