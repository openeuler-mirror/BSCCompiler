#include <stdio.h>

__thread int a = 0;

int main() {
  // CHECK: mrs	{{x[0-9]}}, tpidr_el0
	// CHECK-NEXT: adrp	{{x[0-9]}}, :gottprel:a
	// CHECK-NEXT: ldr	{{x[0-9]}}, [{{x[0-9]}}, #:gottprel_lo12:a]
  // CHECK-NEXT: add	{{x[0-9]}}, {{x[0-9]}}, {{x[0-9]}}
  *(&a) = 1;
  printf("%d\n", a);
}