#include "neon.h"

int main() {
  print_int16_t(
    vuqaddh_s16(
      set_int16_t(),
      set_uint16_t()));
  return 0;
}
