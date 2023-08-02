int printf(const char *, ...);
#pragma pack(1)
struct a {
  unsigned : 3;
  short b;
} __attribute__((aligned(32)));
struct c {
  struct a d;
} e;
struct {
  unsigned : 4;
  struct c f;
} g, h;
struct a *l = &e.d;
struct a m() {
  g = g;
  return h.f.d;
}
void n() { *l = m(); }
int main() {
  n();
  printf("%d\n", e.d.b);
}