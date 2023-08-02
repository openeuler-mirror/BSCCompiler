int bar(int *y);

__attribute__((noinline))
int foo(int *x, int z) {
  int y = 10;
  int *p;
  if (z > 0) {
    p = x;
  } else {
    p = &y;
  }
  int w = bar(p);
  return *p + *x;
}

__attribute__((noinline))
int bar(int *y) {
  return 1;
}

__attribute__((noinline))
int call() {
  int *p = 0;
  return foo(p, 10);
}
