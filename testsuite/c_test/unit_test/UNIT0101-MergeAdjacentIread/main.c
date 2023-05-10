struct A {
  int a;
  int b;
  short c;
  short d;
  short e;
};

__attribute__((noinline))
int func1(struct A *aa1, struct A *aa2) {
  // CHECK-NOT: and
  // CHECK: cmp
  // CHECK: cmp
  // CHECK-NOT: cmp
  return (aa1->a == aa2->a && aa1->b == aa2->b && aa1->c == aa2->c && aa1->d == aa2->d);
}

__attribute__((noinline))
int func2(struct A *aa1, struct A *aa2) {
  // CHECK-NOT: and
  // CHECK: cmp
  // CHECK: cmp
  // CHECK-NOT: cmp
  return (aa1->a == aa2->a && aa1->b == aa2->b && aa1->c == aa2->c && aa1->d == aa2->d && aa1->e == aa2->e);
}

__attribute__((noinline))
int func3(struct A *aa1, struct A *aa2) {
  // CHECK-NOT: and
  // CHECK: cmp
  // CHECK: cmp
  // CHECK-NOT: cmp
  return (aa1->a == aa2->a && aa1->b == aa2->b && aa1->c == aa2->c);
}

__attribute__((noinline))
int func4(struct A *aa1, struct A *aa2) {
  // CHECK-NOT: and
  // CHECK: cmp
  // CHECK: cmp
  // CHECK: cmp
  return (aa1->b == aa2->b && aa1->a == aa2->a && aa1->c == aa2->c);
}

__attribute__((noinline))
int func5(struct A *aa1, struct A *aa2) {
  // CHECK: and
  // CHECK: cmp
  // CHECK-NOT: cmp
  return (aa1->b == aa2->b && aa1->c == aa2->c);
}

__attribute__((noinline))
int func6(struct A *aa1, struct A *aa2) {
  // CHECK-NOT: and
  // CHECK: cmp
  // CHECK-NOT: cmp
  return (aa1->c == aa2->c && aa1->d == aa2->d);
}

__attribute__((noinline))
int func7(struct A *aa1, struct A *aa2) {
  // CHECK: and
  // CHECK: cmp
  // CHECK-NOT: cmp
  return (aa1->c == aa2->c && aa1->d == aa2->d && aa1->e == aa2->e);
}

int main() {
  struct A  aa1 = {1,1,1,1,1};
  struct A  aa2 = {1,1,1,1,1};
  struct A  aa3 = {1,1,1,1,1};
  struct A  aa4 = {0,1,1,1,1};
  struct A  aa5 = {1,1,1,1,1};
  struct A  aa6 = {1,0,1,1,1};
  struct A  aa7 = {1,1,1,1,1};
  struct A  aa8 = {1,1,0,1,1};
  struct A  aa9 = {1,1,1,1,1};
  struct A  aa10 = {1,1,1,0,1};
  struct A  aa11 = {1,1,1,1,1};
  struct A  aa12 = {1,1,1,1,0};

  printf("=============func1=============\n");
  printf("%d\n", func1(&aa1, &aa2));
  printf("%d\n", func1(&aa3, &aa4));
  printf("%d\n", func1(&aa5, &aa6));
  printf("%d\n", func1(&aa7, &aa8));
  printf("%d\n", func1(&aa9, &aa10));
  printf("%d\n", func1(&aa11, &aa12));
  printf("=============func2=============\n");
  printf("%d\n", func2(&aa1, &aa2));
  printf("%d\n", func2(&aa3, &aa4));
  printf("%d\n", func2(&aa5, &aa6));
  printf("%d\n", func2(&aa7, &aa8));
  printf("%d\n", func2(&aa9, &aa10));
  printf("%d\n", func2(&aa11, &aa12));
  printf("=============func3=============\n");
  printf("%d\n", func3(&aa1, &aa2));
  printf("%d\n", func3(&aa3, &aa4));
  printf("%d\n", func3(&aa5, &aa6));
  printf("%d\n", func3(&aa7, &aa8));
  printf("%d\n", func3(&aa9, &aa10));
  printf("%d\n", func3(&aa11, &aa12));
  printf("=============func4=============\n");
  printf("%d\n", func4(&aa1, &aa2));
  printf("%d\n", func4(&aa3, &aa4));
  printf("%d\n", func4(&aa5, &aa6));
  printf("%d\n", func4(&aa7, &aa8));
  printf("%d\n", func4(&aa9, &aa10));
  printf("%d\n", func4(&aa11, &aa12));
  printf("=============func5=============\n");
  printf("%d\n", func5(&aa1, &aa2));
  printf("%d\n", func5(&aa3, &aa4));
  printf("%d\n", func5(&aa5, &aa6));
  printf("%d\n", func5(&aa7, &aa8));
  printf("%d\n", func5(&aa9, &aa10));
  printf("%d\n", func5(&aa11, &aa12));
  printf("=============func6=============\n");
  printf("%d\n", func6(&aa1, &aa2));
  printf("%d\n", func6(&aa3, &aa4));
  printf("%d\n", func6(&aa5, &aa6));
  printf("%d\n", func6(&aa7, &aa8));
  printf("%d\n", func6(&aa9, &aa10));
  printf("%d\n", func6(&aa11, &aa12));
  printf("=============func7=============\n");
  printf("%d\n", func7(&aa1, &aa2));
  printf("%d\n", func7(&aa3, &aa4));
  printf("%d\n", func7(&aa5, &aa6));
  printf("%d\n", func7(&aa7, &aa8));
  printf("%d\n", func7(&aa9, &aa10));
  printf("%d\n", func7(&aa11, &aa12));
  return 0;
}
