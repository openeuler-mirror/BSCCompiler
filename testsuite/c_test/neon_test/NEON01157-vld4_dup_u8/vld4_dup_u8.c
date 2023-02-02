#include "neon.h"

int main() {
  print_uint8x8x4_t(
    vld4_dup_u8(
      set_uint8_t_ptr(32)));
  return 0;
}
