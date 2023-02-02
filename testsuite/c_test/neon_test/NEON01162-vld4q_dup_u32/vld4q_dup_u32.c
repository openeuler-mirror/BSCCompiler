#include "neon.h"

int main() {
  print_uint32x4x4_t(
    vld4q_dup_u32(
      set_uint32_t_ptr(16)));
  return 0;
}
