#include "neon.h"

int main() {
  print_int8x8_t(
    vrsubhn_s16(
      set_int16x8_t(),
      set_int16x8_t()));
  return 0;
}
