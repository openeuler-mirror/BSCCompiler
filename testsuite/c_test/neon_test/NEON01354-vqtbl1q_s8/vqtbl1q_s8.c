#include "neon.h"

int main() {
  print_int8x16_t(
    vqtbl1q_s8(
      set_int8x16_t(),
      set_uint8x16_t()));
  return 0;
}
