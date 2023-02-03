#include "neon.h"

int main() {
  print_int16x4_t(
    vsra_n_s16(
      set_int16x4_t(),
      set_int16x4_t(),
      1));
  return 0;
}
