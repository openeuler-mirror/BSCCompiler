#include "neon.h"

int main() {
  print_int64x1_t(
    vqshl_s64(
      set_int64x1_t(),
      set_int64x1_t()));
  return 0;
}
