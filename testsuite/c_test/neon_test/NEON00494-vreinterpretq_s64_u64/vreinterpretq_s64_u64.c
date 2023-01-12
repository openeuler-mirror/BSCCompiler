#include "neon.h"

int main() {
  print_int64x2_t(
    vreinterpretq_s64_u64(
      set_uint64x2_t()));
  return 0;
}
