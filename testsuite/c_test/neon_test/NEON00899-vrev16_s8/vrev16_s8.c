#include "neon.h"

int main() {
  print_int8x8_t(
    vrev16_s8(
      set_int8x8_t()));
  return 0;
}
