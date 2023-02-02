#include "neon.h"

int main() {
  print_int32x2x4_t(
    vld4_dup_s32(
      set_int32_t_ptr(8)));
  return 0;
}
