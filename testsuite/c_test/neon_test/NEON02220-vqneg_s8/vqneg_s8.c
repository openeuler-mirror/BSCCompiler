#include "neon.h"

int main() {
  print_int8x8_t(
    vqneg_s8(
      set_int8x8_t()));
  return 0;
}
