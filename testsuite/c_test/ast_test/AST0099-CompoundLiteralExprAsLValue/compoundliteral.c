#include <stdio.h>

struct S {
  float a;
};

union U {
  int a;
};

void Foo1(int a) {
  printf("a=%d\n", a);
}

void Foo2(struct S f) {
  printf("f.a=%f\n", f.a);
}

void Foo3(union U u) {
  printf("u.a=%d\n", u.a);
}

int main() {
  Foo1((int){7} = 407);
  Foo2((struct S) {3} = (struct S){222});
  Foo3((union U) {123} = (union U){321});

  return 0;
}
