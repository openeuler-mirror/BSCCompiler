#include "neon.h"

int main() {
  print_int8x16_t(
    vshlq_n_s8(
      set_int8x16_t(),
      set_int()));
  return 0;
}
