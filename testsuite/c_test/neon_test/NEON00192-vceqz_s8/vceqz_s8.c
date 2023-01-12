#include "neon.h"

int main() {
  print_uint8x8_t(
    vceqz_s8(
      set_int8x8_t()));
  return 0;
}
