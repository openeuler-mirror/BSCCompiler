#include "neon.h"

int main() {
  print_uint16x4_t(
    vget_high_u16(
      set_uint16x8_t()));
  return 0;
}
