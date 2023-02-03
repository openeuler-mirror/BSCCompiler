#include "neon.h"

int main() {
  print_uint8_t(
    vqrshrunh_n_s16(
      set_int16_t(),
      1));
  return 0;
}
