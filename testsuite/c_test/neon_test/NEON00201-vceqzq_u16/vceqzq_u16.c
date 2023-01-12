#include "neon.h"

int main() {
  print_uint16x8_t(
    vceqzq_u16(
      set_uint16x8_t()));
  return 0;
}
