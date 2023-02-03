#include "neon.h"

int main() {
  print_uint16_t(
    vqshluh_n_s16(
      set_int16_t(),
      1));
  return 0;
}
