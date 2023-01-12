#include "neon.h"

int main() {
  print_uint8x16_t(
    vcltq_s8(
      set_int8x16_t(),
      set_int8x16_t()));
  return 0;
}
