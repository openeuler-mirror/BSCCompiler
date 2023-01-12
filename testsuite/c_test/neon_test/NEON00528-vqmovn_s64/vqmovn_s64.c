#include "neon.h"

int main() {
  print_int32x2_t(
    vqmovn_s64(
      set_int64x2_t()));
  return 0;
}
