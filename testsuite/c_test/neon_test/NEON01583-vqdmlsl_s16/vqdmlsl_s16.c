#include "neon.h"

int main() {
  print_int32x4_t(
    vqdmlsl_s16(
      set_int32x4_t(),
      set_int16x4_t(),
      set_int16x4_t()));
  return 0;
}