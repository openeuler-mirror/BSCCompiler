int printf(const char *, ...);
int a, b, d, c, g;
int e[];
int *h();
void i() {
  int *f = &e[2];
  h(f);
  *f = f == 0;
}
int *h() {
  for (;; d++) {
    for (; b > -14; b--)
      ;
    return &g;
  }
}
int main() {
  i();
  printf("%d\n", a);
}
