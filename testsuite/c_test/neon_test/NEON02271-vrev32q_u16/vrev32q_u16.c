#include "neon.h"

int main() {
  print_uint16x8_t(
    vrev32q_u16(
      set_uint16x8_t()));
  return 0;
}
