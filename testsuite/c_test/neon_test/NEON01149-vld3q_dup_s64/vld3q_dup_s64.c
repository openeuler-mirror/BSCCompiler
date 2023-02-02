#include "neon.h"

int main() {
  print_int64x2x3_t(
    vld3q_dup_s64(
      set_int64_t_ptr(6)));
  return 0;
}
