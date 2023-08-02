int printf(const char *, ...);
int a;
long b;
long *c = &b;
short(d)(e, f) { return e - f; }
int main() {
  unsigned g = 4294967295;
  a = 23;
  for (; a > 5; a = d(a, 6))
    ++g;
  *c = g;
  printf("%d\n", (int)b);
}
