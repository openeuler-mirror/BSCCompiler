#include "neon.h"

int main() {
  print_uint8x8x2_t(
    vld2_u8(
      set_uint8_t_ptr(16)));
  return 0;
}
