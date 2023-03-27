#include <stdio.h>
#include <string.h>

struct StructReturnType {
  /* passing this struct in registers. */
  int a1;
};

struct StructReturnType FuncTest(void);

int main() {
  // CHECK-NOT: __stack_chk_guard
  int res = FuncTest().a1;
  return res;
}
