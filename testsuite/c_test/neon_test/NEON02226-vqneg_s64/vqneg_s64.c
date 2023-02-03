#include "neon.h"

int main() {
  print_int64x1_t(
    vqneg_s64(
      set_int64x1_t()));
  return 0;
}
