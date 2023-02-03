#include "neon.h"

int main() {
  print_uint8x8_t(
    vqshlu_n_s8(
      set_int8x8_t(),
      1));
  return 0;
}
