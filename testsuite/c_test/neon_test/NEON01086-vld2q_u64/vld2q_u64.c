#include "neon.h"

int main() {
  print_uint64x2x2_t(
    vld2q_u64(
      set_uint64_t_ptr(4)));
  return 0;
}
