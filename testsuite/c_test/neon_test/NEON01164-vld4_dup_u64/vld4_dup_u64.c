#include "neon.h"

int main() {
  print_uint64x1x4_t(
    vld4_dup_u64(
      set_uint64_t_ptr(4)));
  return 0;
}
