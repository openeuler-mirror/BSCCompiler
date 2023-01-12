#include "neon.h"

int main() {
  print_uint64x1_t(
    vceqz_s64(
      set_int64x1_t()));
  return 0;
}
