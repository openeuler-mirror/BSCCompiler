#include "neon.h"

int main() {
  print_int8_t(
    vqrshlb_s8(
      set_int8_t(),
      set_int8_t()));
  return 0;
}
