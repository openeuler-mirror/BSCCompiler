#include "neon.h"

int main() {
  print_uint8x16_t(
    vclzq_u8(
      set_uint8x16_t()));
  return 0;
}
