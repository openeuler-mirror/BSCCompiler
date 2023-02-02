#include "neon.h"

int main() {
  print_uint64x2x4_t(
    vld4q_dup_u64(
      set_uint64_t_ptr(8)));
  return 0;
}
