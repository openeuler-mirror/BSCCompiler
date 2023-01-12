#include "neon.h"

int main() {
  print_int8x16_t(
    vabdq_s8(
      set_int8x16_t(),
      set_int8x16_t()));
  return 0;
}
