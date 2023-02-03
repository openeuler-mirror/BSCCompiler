#include "neon.h"

int main() {
  print_uint16x4_t(
    vqshlu_n_s16(
      set_int16x4_t(),
      1));
  return 0;
}
