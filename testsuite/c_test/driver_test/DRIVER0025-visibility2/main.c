#include <stdio.h>

extern void externFunc();
// CHECK-NOT: .hidden	externFunc
extern int a;
// CHECK-NOT: .hidden	a

static int staticFunc() {
    return 2;
}
// CHECK-NOT: .hidden	staticFunc

__attribute((visibility("protected"))) int protectedFunc() {
  return 8;
}
// CHECK: .protected	protectedFunc
// CHECK-NOT: .hidden	protectedFunc

int globalFunc(int a) {
  return a + 1;
}
// CHECK: .hidden	globalFunc

int main() {
  externFunc();
  printf("%d", staticFunc() + globalFunc(3) + protectedFunc() + a);

  return 0;
}