#include "neon.h"

int main() {
  print_int8x8_t(
    vtbl4_s8(
      set_int8x8x4_t(),
      set_int8x8_t()));
  return 0;
}
