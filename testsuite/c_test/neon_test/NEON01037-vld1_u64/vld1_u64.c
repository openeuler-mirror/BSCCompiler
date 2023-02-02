#include "neon.h"

int main() {
  print_uint64x1_t(
    vld1_u64(
      set_uint64_t_ptr(1)));
  return 0;
}
