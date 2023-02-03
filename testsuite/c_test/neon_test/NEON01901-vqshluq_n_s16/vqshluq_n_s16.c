#include "neon.h"

int main() {
  print_uint16x8_t(
    vqshluq_n_s16(
      set_int16x8_t(),
      1));
  return 0;
}
