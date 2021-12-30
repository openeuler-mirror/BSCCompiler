int bar(int *y);

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

int bar(int *y) {
  return 1;
}

int call() {
  int *p = 0;
  return foo(p, 10);
}
