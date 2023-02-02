#include "neon.h"

int main() {
  uint64_t ptr[6] = { 0 };
  vst3q_u64(
      ptr,
      set_uint64x2x3_t());
  print_uint64_t_ptr(ptr, 6);
  return 0;
}
