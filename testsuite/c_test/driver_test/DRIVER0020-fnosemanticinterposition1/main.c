#include <stdio.h>

extern void externFunc();
static int staticFunc() {
    return 2;
}
__attribute((visibility("hidden"))) int hiddenFunc() {
  return 8;
}
// CHECK-NOT: .set	hiddenFunc.localalias, hiddenFunc

int globalFunc(int a) {
  return a + 1;
}
// CHECK: .set	globalFunc.localalias, globalFunc

int main() {
  externFunc();
  // CHECK: bl	externFunc 

  printf("%d", staticFunc() + globalFunc(3) + hiddenFunc());
  // CHECK: bl	staticFunc
	// CHECK: bl	globalFunc.localalias
  // CHECK: bl	hiddenFunc
  // CHECK-NOT: bl	hiddenFunc.localalias

  return 0;
}