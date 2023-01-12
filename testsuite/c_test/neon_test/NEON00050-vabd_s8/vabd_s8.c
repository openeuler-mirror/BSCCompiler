#include "neon.h"

int main() {
  print_int8x8_t(
    vabd_s8(
      set_int8x8_t(),
      set_int8x8_t()));
  return 0;
}
