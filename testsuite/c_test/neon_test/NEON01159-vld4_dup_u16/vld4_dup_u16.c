#include "neon.h"

int main() {
  print_uint16x4x4_t(
    vld4_dup_u16(
      set_uint16_t_ptr(16)));
  return 0;
}
