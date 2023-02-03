#include "neon.h"

int main() {
  print_uint16x4_t(
    vsqadd_u16(
      set_uint16x4_t(),
      set_int16x4_t()));
  return 0;
}
