#include "neon.h"

int main() {
  print_int32x4_t(
    vld1q_dup_s32(
      set_int32_t_ptr(4)));
  return 0;
}
