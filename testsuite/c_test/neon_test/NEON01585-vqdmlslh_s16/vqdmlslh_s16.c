#include "neon.h"

int main() {
  print_int32_t(
    vqdmlslh_s16(
      set_int32_t(),
      set_int16_t(),
      set_int16_t()));
  return 0;
}
