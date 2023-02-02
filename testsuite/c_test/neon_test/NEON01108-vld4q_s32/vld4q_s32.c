#include "neon.h"

int main() {
  print_int32x4x4_t(
    vld4q_s32(
      set_int32_t_ptr(16)));
  return 0;
}
