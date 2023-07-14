#include <stdint.h>

a = 208, b = 5;
c() {
  int8_t *d = &b;
  // CHECK: adrp	{{x[0-9]}}, b
  // CHECK: add	{{x[0-9]}}, {{x[0-9]}}, :lo12:b
  // CHECK-NEXT: ldr	{{x[0-9]}}, [{{x[0-9]}}]
  // CHECK: str	{{x[0-9]}}, [{{x[0-9]}}]
  uint64_t *e;
  __atomic_compare_exchange_n(e, d, 0, 0, 0, 0);
}
main() {}