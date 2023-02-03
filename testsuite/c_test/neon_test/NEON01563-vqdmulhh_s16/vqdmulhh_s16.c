#include "neon.h"

int main() {
  print_int16_t(
    vqdmulhh_s16(
      set_int16_t(),
      set_int16_t()));
  return 0;
}
