#include "neon.h"

int main() {
  print_int16x8_t(
    vpaddlq_s8(
      set_int8x16_t()));
  return 0;
}
