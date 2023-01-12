#include "neon.h"

int main() {
  print_int16x4_t(
    vrev64_s16(
      set_int16x4_t()));
  return 0;
}
