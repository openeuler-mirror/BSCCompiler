#include "neon.h"

int main() {
  print_int32x2x2_t(
    vld2_dup_s32(
      set_int32_t_ptr(4)));
  return 0;
}
