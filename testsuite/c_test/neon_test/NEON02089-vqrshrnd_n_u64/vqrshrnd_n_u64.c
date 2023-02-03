#include "neon.h"

int main() {
  print_uint32_t(
    vqrshrnd_n_u64(
      set_uint64_t(),
      1));
  return 0;
}
