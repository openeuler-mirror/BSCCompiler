#include "neon.h"

int main() {
  print_int16x8_t(
    vshlq_n_s16(
      set_int16x8_t(),
      set_int()));
  return 0;
}
