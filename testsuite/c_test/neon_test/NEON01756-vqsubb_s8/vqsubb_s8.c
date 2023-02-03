#include "neon.h"

int main() {
  print_int8_t(
    vqsubb_s8(
      set_int8_t(),
      set_int8_t()));
  return 0;
}
