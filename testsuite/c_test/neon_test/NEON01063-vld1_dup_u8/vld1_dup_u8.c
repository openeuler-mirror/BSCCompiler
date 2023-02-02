#include "neon.h"

int main() {
  print_uint8x8_t(
    vld1_dup_u8(
      set_uint8_t_ptr(8)));
  return 0;
}
