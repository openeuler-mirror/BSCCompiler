#include "neon.h"

int main() {
  print_uint64x2_t(
    vcgtzq_s64(
      set_int64x2_t()));
  return 0;
}
