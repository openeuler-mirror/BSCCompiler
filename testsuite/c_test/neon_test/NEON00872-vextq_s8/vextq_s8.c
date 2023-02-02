#include "neon.h"

int main() {
  print_int8x16_t(
    vextq_s8(
      set_int8x16_t(),
      set_int8x16_t(),
      1));
  return 0;
}
