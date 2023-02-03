#include "neon.h"

int main() {
  print_int8_t(
    vqshlb_n_s8(
      set_int8_t(),
      1));
  return 0;
}
