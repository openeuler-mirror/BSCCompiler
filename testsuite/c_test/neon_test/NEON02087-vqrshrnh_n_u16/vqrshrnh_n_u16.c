#include "neon.h"

int main() {
  print_uint8_t(
    vqrshrnh_n_u16(
      set_uint16_t(),
      1));
  return 0;
}
