#include "neon.h"

int main() {
  print_uint8x8_t(
    vqmovun_s16(
      set_int16x8_t()));
  return 0;
}
