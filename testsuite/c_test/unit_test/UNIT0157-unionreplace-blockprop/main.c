union A {
  struct {
    short a;
    char b;
    char c;
  } x;
  int y;
};

union A bar() {
 union A tmp;
 tmp.y = -1;
 tmp.x.c = 0;
 return tmp;
}

__attribute__ ((noinline))
union A foo() {
  union A tmp = bar();
  return tmp;
}

int main() {
  union A a = foo();
  if (a.y != 0xffffff) {
    abort();
  }
  return 0;
}
