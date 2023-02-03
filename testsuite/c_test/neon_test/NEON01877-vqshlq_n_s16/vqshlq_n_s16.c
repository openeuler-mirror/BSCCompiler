#include "neon.h"

int main() {
  print_int16x8_t(
    vqshlq_n_s16(
      set_int16x8_t(),
      1));
  return 0;
}
