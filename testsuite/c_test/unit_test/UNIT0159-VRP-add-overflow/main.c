#include <stdio.h>
#include <stdlib.h>
 
__attribute__((__noinline__)) unsigned int GetNum() {
  return 22;
}
 
int main() {
  unsigned int a = GetNum();
  if (a == 20 || a == 21) {
  } else {
    // CHECK: br
    if (a > 15) {
      // CHECK: br
      if (a < 18) {
        // CHECK: printf
        printf("succ\n");
      } else {
        // CHECK: printf
        printf("err\n"); // can not delete this edge in vrp
      }
    }
  }
  return 0;
}
