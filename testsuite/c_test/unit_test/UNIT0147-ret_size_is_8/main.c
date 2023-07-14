struct {
  char a;
} b;
struct c {
  char : 1;
} e, f;
int d, g, h;
char *i;
struct c j() {
  for (; h;) g = d = 0;
  return e;
}
int main() {
  char k = 10;
  f = j();
  i = &k;
  b.a = *i;
  printf("%d\n", b);
}