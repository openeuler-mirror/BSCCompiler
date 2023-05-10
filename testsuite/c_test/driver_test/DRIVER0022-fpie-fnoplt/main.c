#include <stdio.h>

__attribute((visibility("hidden"))) int Bar1() {
  return 7;
}

int Bar2(int a)
{
  int b = 2;
  return a + b;
}

static int Bar3(int a)
{
  int b = 3;
  return a + b;
}

int main()
{
  int j = Bar1() + Bar2(4);
  // CHECK: bl	Bar1
  // CHECK: bl	Bar2
  int k = Bar3(5);
  // CHECK: bl	Bar3
  printf("out = %d\n", j + k);
  // CHECK: adrp	{{x[0-9]}}, :got:printf
  // CHECK-NEXT: ldr {{x[0-9]}}, [{{x[0-9]}}, #:got_lo12:printf]
}