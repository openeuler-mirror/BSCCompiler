#include "neon.h"

int main() {
  print_uint16x4_t(
    vclt_s16(
      set_int16x4_t(),
      set_int16x4_t()));
  return 0;
}
