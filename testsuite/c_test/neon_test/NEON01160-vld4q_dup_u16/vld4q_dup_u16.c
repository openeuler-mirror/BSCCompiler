#include "neon.h"

int main() {
  print_uint16x8x4_t(
    vld4q_dup_u16(
      set_uint16_t_ptr(32)));
  return 0;
}
