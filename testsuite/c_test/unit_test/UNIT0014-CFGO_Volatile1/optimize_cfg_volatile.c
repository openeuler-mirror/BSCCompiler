#include <stdio.h>
int main() {
  volatile int b[10], i;
  for (i = 0; i < 10; i++) {
    b[i] = i << 2;
  }
  for (i = 0; i < 10; i++) {
    printf("%d %d\n", i, b[i]);
  }
}
