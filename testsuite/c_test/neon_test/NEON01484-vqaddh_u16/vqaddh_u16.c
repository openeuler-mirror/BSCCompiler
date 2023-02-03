#include "neon.h"

int main() {
  print_uint16_t(
    vqaddh_u16(
      set_uint16_t(),
      set_uint16_t()));
  return 0;
}
