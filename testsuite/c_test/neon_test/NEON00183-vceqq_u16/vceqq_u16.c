#include "neon.h"

int main() {
  print_uint16x8_t(
    vceqq_u16(
      set_uint16x8_t(),
      set_uint16x8_t()));
  return 0;
}
