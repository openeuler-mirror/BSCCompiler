#include "neon.h"

int main() {
  print_int16_t(
    vqrshrns_n_s32(
      set_int32_t(),
      1));
  return 0;
}
