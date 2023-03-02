#include <stdio.h>
int g = 0;

struct A {
  int *a;
};

struct A foo() {
  return (struct A){ &g };
}

int main () {
  struct A x = foo();
  (*(x.a))++;
  printf("%d\n", g);
  return 0;
}
