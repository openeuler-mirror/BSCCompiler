#include "neon.h"

int main() {
  print_int16x4_t(
    vuqadd_s16(
      set_int16x4_t(),
      set_uint16x4_t()));
  return 0;
}
