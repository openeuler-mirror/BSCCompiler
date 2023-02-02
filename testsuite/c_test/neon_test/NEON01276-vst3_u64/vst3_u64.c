#include "neon.h"

int main() {
  uint64_t ptr[3] = { 0 };
  vst3_u64(
      ptr,
      set_uint64x1x3_t());
  print_uint64_t_ptr(ptr, 3);
  return 0;
}
