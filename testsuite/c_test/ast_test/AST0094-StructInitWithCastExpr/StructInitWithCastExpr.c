#include <stdio.h>
struct T { int i; };
struct S { struct T t; };
static struct S s = (struct S){ .t = { 42 } };
struct T t = (struct T){.i = 22};
int main() {
  printf("s.t.i=%d\n", s.t.i);
  printf("t.i=%d\n", t.i);
}
