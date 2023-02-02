#include "neon.h"

int main() {
  print_int32x2x3_t(
    vld3_dup_s32(
      set_int32_t_ptr(6)));
  return 0;
}
