#include <stdint.h>

a = 3, b = 2;
c() {
  uint64_t d;
  uint64_t *e;
  // CHECK: adrp	{{x[0-9]}}, b
  // CHECK: add	{{x[0-9]}}, {{x[0-9]}}, :lo12:b
  // CHECK-NEXT: ldr	{{x[0-9]}}, [{{x[0-9]}}]
  int32_t *f = &b;
  __atomic_compare_exchange_n(e, f, 0, a, d, 0);
  uint64_t *g;
  f + *g &&c;
}
main() {}