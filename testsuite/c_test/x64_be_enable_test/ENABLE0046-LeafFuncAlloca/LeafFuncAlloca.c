_Bool foo() {
  int x0 = 0, x1 = 1, x2 = 2, x3 = 3, x4 = 4;
  int *p = __builtin_alloca(5 * sizeof(int));
  p[0] = 0;
  p[1] = 1;
  p[2] = 2;
  p[3] = 3;
  p[4] = 4;

  return (x0 == 0 && x1 == 1 && x2 == 2 && x3 == 3 && x4 == 4 && p[0] == 0 &&
          p[1] == 1 && p[2] == 2 && p[3] == 3 && p[4] == 4);
}

int main() {
  if (!foo()) {
    abort();
  }
  return 0;
}
