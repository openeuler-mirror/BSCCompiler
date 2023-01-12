#include "neon.h"

int main() {
  print_int8x8_t(
    vmovn_s16(
      set_int16x8_t()));
  return 0;
}
