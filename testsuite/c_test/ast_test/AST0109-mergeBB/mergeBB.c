#include<stdio.h>

int __attribute__ ((noinline)) test(int a, float b, int c, float d, int e, float f) {
  if (a == 0) {
    return c;
  }

  if (b < f) {
    return a;
  }

  if (d > b) {
    return e;
  }

  if (a > c) {
    return c + a;
  }

  if (d > f + b) {
    return e + a;
  }

  return a + c + e;
}


int main() {
  int ans = test(2, 2.5, 4, 5.6, 6, 7.8);
  printf("%d\n", ans);
}
