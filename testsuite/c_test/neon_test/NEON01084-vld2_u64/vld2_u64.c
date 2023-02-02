#include "neon.h"

int main() {
  print_uint64x1x2_t(
    vld2_u64(
      set_uint64_t_ptr(2)));
  return 0;
}
