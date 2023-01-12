#include "neon.h"

int main() {
  print_uint8x8_t(
    vtst_s8(
      set_int8x8_t(),
      set_int8x8_t()));
  return 0;
}
