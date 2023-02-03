#include "neon.h"

int main() {
  print_int64x2_t(
    vqabsq_s64(
      set_int64x2_t()));
  return 0;
}
