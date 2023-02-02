#include "neon.h"

int main() {
  print_int64x1x2_t(
    vld2_dup_s64(
      set_int64_t_ptr(2)));
  return 0;
}
