#include "neon.h"

int main() {
  print_int32x4x2_t(
    vld2q_dup_s32(
      set_int32_t_ptr(8)));
  return 0;
}
