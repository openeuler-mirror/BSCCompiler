#include "neon.h"

int main() {
  print_int16x8_t(
    vextq_s16(
      set_int16x8_t(),
      set_int16x8_t(),
      set_int()));
  return 0;
}
