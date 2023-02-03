#include "neon.h"

int main() {
  print_uint8_t(
    vqshlub_n_s8(
      set_int8_t(),
      1));
  return 0;
}
