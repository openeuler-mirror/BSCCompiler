#include "neon.h"

int main() {
  print_int16x4_t(
    vpaddl_s8(
      set_int8x8_t()));
  return 0;
}
