#include "neon.h"

int main() {
  print_uint16_t(
    vqshlh_n_u16(
      set_uint16_t(),
      1));
  return 0;
}
