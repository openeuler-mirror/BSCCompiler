#include "neon.h"

int main() {
  print_int32x2_t(
    vpadal_s16(
      set_int32x2_t(),
      set_int16x4_t()));
  return 0;
}
