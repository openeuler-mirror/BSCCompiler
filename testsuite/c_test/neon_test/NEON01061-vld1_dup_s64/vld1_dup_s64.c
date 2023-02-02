#include "neon.h"

int main() {
  print_int64x1_t(
    vld1_dup_s64(
      set_int64_t_ptr(1)));
  return 0;
}
