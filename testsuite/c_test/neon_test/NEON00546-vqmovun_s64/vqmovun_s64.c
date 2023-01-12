#include "neon.h"

int main() {
  print_uint32x2_t(
    vqmovun_s64(
      set_int64x2_t()));
  return 0;
}
