#include <stdio.h>

__attribute((visibility("hidden"))) extern void externFunc();
// CHECK: .hidden	externFunc
__attribute((visibility("protected"))) extern int protectedVar;
__attribute((visibility("hidden"))) extern int hiddenVar;

static int staticFunc() {
    return 2;
}
__attribute((visibility("protected"))) int protectedFunc() {
  return 8;
}
// CHECK: .protected	protectedFunc

int globalFunc(int a) {
  return a + 1;
}

int main() {
  externFunc();
  printf("%d", staticFunc() + globalFunc(3) + protectedFunc() + protectedVar + hiddenVar);

  return 0;
}
// CHECK: .protected	protectedVar
// CHECK: .local	hiddenVar