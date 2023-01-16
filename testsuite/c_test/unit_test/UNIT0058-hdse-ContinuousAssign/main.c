struct A {
  int a;
  int b;
  int c;
  int d;
};

#pragma pack(1)
struct B {
  short a1;
  int   a2;
  short b1;
  short b2;
  short c1;
  short c2;
  short d1;
};

__attribute__ ((noinline))
void foo(struct A *a) {
  ((struct B *)(a))->a2 = 12485;
  ((struct A *)(a))->b = 3680;
}

int main () {
  struct A a = { 0, 0, 0, 0 };
  foo(&a);
  if (a.a != 818216960 || a.b != 3680) {
    abort();
  }
  return 0;
}
