#include "neon.h"

int main() {
  print_uint16_t(
    vqrshrns_n_u32(
      set_uint32_t(),
      1));
  return 0;
}
