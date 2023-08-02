#include <stdio.h>

#define test(_si1, _si2)                      \
  ({                                          \
    int si1 = (_si1);                         \
    int si2 = (_si2);                         \
    (((si1) > (0)) && ((si2) > (0))) ? 1 : 0; \
  })

void func_1(int a, int b) {
  if (test(a,b)) {
    printf("%d\n", 1);
  } else {
    printf("%d\n", 0);
  }
}

int main() {
  func_1(1,1);
  func_1(1,-1);
  return 0;
}
