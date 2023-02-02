#include "neon.h"

int main() {
  print_int64x2x4_t(
    vld4q_s64(
      set_int64_t_ptr(8)));
  return 0;
}
