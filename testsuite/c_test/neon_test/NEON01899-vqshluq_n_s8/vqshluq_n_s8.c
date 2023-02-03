#include "neon.h"

int main() {
  print_uint8x16_t(
    vqshluq_n_s8(
      set_int8x16_t(),
      1));
  return 0;
}
