#include "neon.h"

int main() {
  print_int8_t(
    vqshrnh_n_s16(
      set_int16_t(),
      1));
  return 0;
}
