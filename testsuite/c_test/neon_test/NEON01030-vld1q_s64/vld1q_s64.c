#include "neon.h"

int main() {
  print_int64x2_t(
    vld1q_s64(
      set_int64_t_ptr(2)));
  return 0;
}
