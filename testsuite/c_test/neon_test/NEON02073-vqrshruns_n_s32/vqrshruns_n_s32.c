#include "neon.h"

int main() {
  print_uint16_t(
    vqrshruns_n_s32(
      set_int32_t(),
      1));
  return 0;
}
