void abort (void);

__thread int t0 = 0x10;
__thread int t1 = 0x10;

int main (int argc, char **argv)
{
  // CHECK: mrs	{{x[0-9]}}, tpidr_el0
	// CHECK-NEXT: add	{{x[0-9]}}, {{x[0-9]}}, #:tprel_hi12:t0, lsl #12
	// CHECK-NEXT: add	{{x[0-9]}}, {{x[0-9]}}, #:tprel_lo12_nc:t0
  // CHECK: mrs	{{x[0-9]}}, tpidr_el0
	// CHECK-NEXT: add	{{x[0-9]}}, {{x[0-9]}}, #:tprel_hi12:t1, lsl #12
	// CHECK-NEXT: add	{{x[0-9]}}, {{x[0-9]}}, #:tprel_lo12_nc:t1
  if (t0 != t1)
    abort();

  return  0;
}