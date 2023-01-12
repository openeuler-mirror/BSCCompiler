#include "neon.h"

int main() {
  print_int8x8_t(
    vshr_n_s8(
      set_int8x8_t(),
      set_int()));
  return 0;
}
