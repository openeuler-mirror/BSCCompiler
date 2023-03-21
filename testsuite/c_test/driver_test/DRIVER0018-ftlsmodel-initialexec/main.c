#include <stdio.h>
void abort (void);

__thread int t0 = 0x10;
__thread int t1 = 0x10;
extern __thread int t2;

// CHECK: mrs	{{x[0-9]}}, tpidr_el0
// CHECK-NEXT: adrp	{{x[0-9]}}, :gottprel:t2
// CHECK-NEXT: ldr	{{x[0-9]}}, [{{x[0-9]}}, #:gottprel_lo12:t2]
void printExternTLS() {
  printf("extern TLS is %d\n", t2);
}

int main (int argc, char **argv)
{
  // CHECK: mrs	{{x[0-9]}}, tpidr_el0
	// CHECK-NEXT: adrp	{{x[0-9]}}, :gottprel:t0
	// CHECK-NEXT: ldr	{{x[0-9]}}, [{{x[0-9]}}, #:gottprel_lo12:t0]
  // CHECK: mrs	{{x[0-9]}}, tpidr_el0
	// CHECK-NEXT: adrp	{{x[0-9]}}, :gottprel:t1
	// CHECK-NEXT: ldr	{{x[0-9]}}, [{{x[0-9]}}, #:gottprel_lo12:t1]
  if (t0 != t1)
    abort();

  return  0;
}