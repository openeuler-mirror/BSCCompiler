#include "neon.h"

int main() {
  print_int16x4_t(
    vabd_s16(
      set_int16x4_t(),
      set_int16x4_t()));
  return 0;
}
