#include "neon.h"

int main() {
  print_int32x2_t(
    vld1_dup_s32(
      set_int32_t_ptr(2)));
  return 0;
}
