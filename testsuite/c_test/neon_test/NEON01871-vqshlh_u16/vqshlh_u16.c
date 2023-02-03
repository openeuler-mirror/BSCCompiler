#include "neon.h"

int main() {
  print_uint16_t(
    vqshlh_u16(
      set_uint16_t(),
      set_int16_t()));
  return 0;
}
