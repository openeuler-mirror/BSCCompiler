#include "neon.h"

int main() {
  print_uint16_t(
    vqsubh_u16(
      set_uint16_t(),
      set_uint16_t()));
  return 0;
}
