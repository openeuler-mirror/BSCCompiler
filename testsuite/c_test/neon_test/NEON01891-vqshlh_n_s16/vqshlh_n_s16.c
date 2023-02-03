#include "neon.h"

int main() {
  print_int16_t(
    vqshlh_n_s16(
      set_int16_t(),
      1));
  return 0;
}
