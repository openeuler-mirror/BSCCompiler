#include "neon.h"

int main() {
  print_uint64_t(
    vceqd_u64(
      set_uint64_t(),
      set_uint64_t()));
  return 0;
}
