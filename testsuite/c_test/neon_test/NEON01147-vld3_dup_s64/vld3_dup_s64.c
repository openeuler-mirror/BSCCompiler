#include "neon.h"

int main() {
  print_int64x1x3_t(
    vld3_dup_s64(
      set_int64_t_ptr(3)));
  return 0;
}
