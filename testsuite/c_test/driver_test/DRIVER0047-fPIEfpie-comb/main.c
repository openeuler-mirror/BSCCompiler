#include <stdint.h>

extern uint64_t a;
int8_t b = 5;

void c() {
  // CHECK: adrp	x0, b
  // CHECK-NEXT: add	x0, x0, :lo12:b
  int8_t *d = &b;
  // CHECK: adrp	x0, _GLOBAL_OFFSET_TABLE_
  // CHECK-NEXT: ldr	x0, [x0, #:gotpage_lo15:a]
  uint64_t *e = &a;
  __atomic_compare_exchange_n(e, d, 0, 0, 0, 0);
}
int main() {
  return 0;
}