#include "neon.h"

int main() {
  print_int8x16_t(
    vqtbl2q_s8(
      set_int8x16x2_t(),
      set_uint8x16_t()));
  return 0;
}
