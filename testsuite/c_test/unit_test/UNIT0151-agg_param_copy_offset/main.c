struct {
  unsigned : 3;
  long a;
  unsigned : 1;
} b;
struct c {
  int d;
} __attribute__((aligned(64)));
struct c e;
void f() {}
int main() {
  f(e, b);
}
