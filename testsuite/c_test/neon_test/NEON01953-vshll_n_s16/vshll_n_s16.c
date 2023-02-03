#include "neon.h"

int main() {
  print_int32x4_t(
    vshll_n_s16(
      set_int16x4_t(),
      1));
  return 0;
}
