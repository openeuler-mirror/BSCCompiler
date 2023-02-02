#include "neon.h"

int main() {
  print_int64x1x4_t(
    vld4_dup_s64(
      set_int64_t_ptr(4)));
  return 0;
}
