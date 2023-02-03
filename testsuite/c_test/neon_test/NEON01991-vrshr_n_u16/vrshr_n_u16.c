#include "neon.h"

int main() {
  print_uint16x4_t(
    vrshr_n_u16(
      set_uint16x4_t(),
      1));
  return 0;
}
