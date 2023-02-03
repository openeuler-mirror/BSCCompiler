#include "neon.h"

int main() {
  print_uint64x2_t(
    vqshlq_u64(
      set_uint64x2_t(),
      set_int64x2_t()));
  return 0;
}
