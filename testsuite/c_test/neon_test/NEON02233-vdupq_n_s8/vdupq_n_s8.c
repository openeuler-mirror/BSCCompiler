#include "neon.h"

int main() {
  print_int8x16_t(
    vdupq_n_s8(
      set_int8_t()));
  return 0;
}
