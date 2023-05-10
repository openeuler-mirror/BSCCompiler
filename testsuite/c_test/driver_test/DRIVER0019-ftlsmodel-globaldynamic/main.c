void abort (void);

__thread int t0 = 0x10;
__thread int t1 = 0x10;

int main (int argc, char **argv)
{
  // CHECK: adrp {{x[0-9]}}, :tlsdesc:t0
	// CHECK-NEXT: ldr {{x[0-9]}}, [{{x[0-9]}}, #:tlsdesc_lo12:t0]
	// CHECK-NEXT: add	{{x[0-9]}}, {{x[0-9]}}, :tlsdesc_lo12:t0
	// CHECK-NEXT: .tlsdesccall	t0
  // CHECK: adrp	{{x[0-9]}}, :tlsdesc:t1
	// CHECK-NEXT: ldr {{x[0-9]}}, [{{x[0-9]}}, #:tlsdesc_lo12:t1]
	// CHECK-NEXT: add	{{x[0-9]}}, {{x[0-9]}}, :tlsdesc_lo12:t1
	// CHECK-NEXT: .tlsdesccall	t1
  if (t0 != t1)
    abort();

  return  0;
}