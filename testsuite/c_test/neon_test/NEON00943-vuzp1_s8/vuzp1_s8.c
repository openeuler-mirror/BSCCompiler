#include "neon.h"

int main() {
  print_int8x8_t(
    vuzp1_s8(
      set_int8x8_t(),
      set_int8x8_t()));
  return 0;
}
