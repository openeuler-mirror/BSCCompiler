#include <stdint.h>

__thread uint64_t a;
int32_t b;
uint8_t uc_9;
int64_t c() {
  int32_t *d = &b;
  // CHECK: adrp	{{x[0-9]}}, _GLOBAL_OFFSET_TABLE_
  // CHECK: ldr	{{x[0-9]}}, [{{x[0-9]}}, #:gotpage_lo15:b]
  uint64_t e = 5;
  uint64_t *f = &e;
  uint8_t g = uc_9;
  __atomic_compare_exchange_n(f, d, 2, g, e, 0);
}
void main() {}