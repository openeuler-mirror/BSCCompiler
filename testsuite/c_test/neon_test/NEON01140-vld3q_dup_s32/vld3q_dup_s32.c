#include "neon.h"

int main() {
  print_int32x4x3_t(
    vld3q_dup_s32(
      set_int32_t_ptr(12)));
  return 0;
}
