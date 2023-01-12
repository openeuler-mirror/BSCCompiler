#include "neon.h"

int main() {
  print_uint8x8_t(
    vcgtz_s8(
      set_int8x8_t()));
  return 0;
}
