#include <stdint.h>

int foo(int a, int b);
int main() {
  int a = sin(1);
  int b = cos(2);
  foo(a, b);
}

// CHECK: Shared library: [libfoo.so.1]
// CHECK: Shared library: [libm.so.6]
// CHECK-NEXT: Shared library: [libpthread.so.0]