#include "neon.h"

int main() {
  print_uint16x8_t(
    vcltq_s16(
      set_int16x8_t(),
      set_int16x8_t()));
  return 0;
}
