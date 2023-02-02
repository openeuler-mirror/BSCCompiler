#include "neon.h"

int main() {
  uint64_t ptr[4] = { 0 };
  vst4_u64(
      ptr,
      set_uint64x1x4_t());
  print_uint64_t_ptr(ptr, 4);
  return 0;
}
