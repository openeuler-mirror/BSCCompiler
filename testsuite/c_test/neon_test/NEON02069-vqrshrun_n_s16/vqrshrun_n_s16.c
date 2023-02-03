#include "neon.h"

int main() {
  print_uint8x8_t(
    vqrshrun_n_s16(
      set_int16x8_t(),
      1));
  return 0;
}
