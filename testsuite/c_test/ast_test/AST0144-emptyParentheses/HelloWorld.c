#include <complex.h>

enum {
  RED = 0,
  BLUE = 1,
  GREEN = 2,
};

struct __attribute__((aligned(1))) S {
    char a;
};

void func1();
void func2();
void Test() {
  unsigned char a;
  //CHECK: call &func1 (cvt i32 u32 (dread u32 %a
  func1(a);
  unsigned int b;
  //CHECK: call &func1 (dread u32 %b
  func1(b);
  unsigned long c;
  //CHECK: call &func1 (dread u64 %c
  func1(c);
  char d;
  //CHECK: call &func1 (cvt i32 u32 (dread u32 %d
  func1(d);
  int e;
  //CHECK: call &func1 (dread i32 %e
  func1(e);
  long g;
  //CHECK: call &func1 (dread i64 %g
  func1(g);
  float f;
  //CHECK: call &func1 (cvt f64 f32 (dread f32 %f
  func1(f);
  double h;
  //CHECK: call &func1 (dread f64 %h
  func1(h);
  //CHECK: call &func2 (cvt i32 u32 (dread u32 %a
  func2(a);
  //CHECK: call &func2 (
  //CHECK-NEXT: cvt i32 u32 (dread u32 %a
  //CHECK-NEXT: dread u32 %b
  //CHECK-NEXT: dread u64 %c
  //CHECK-NEXT: cvt i32 u32 (dread u32 %d
  func2(a, b, c, d);
  struct S s;
  //CHECK: call &func2 (dread agg %s
  func2(s);
  int *p;
  //CHECK: call &func2 (dread ptr %p
  func2(p);
  //CHECK: call &func2 (constval i32 0)
  func2(RED);
  //CHECK: call &func2 (dread agg %Complex
  func2(1 + I);
  //CHECK: call &func2 (cvt i32 u32 (dread u32 %postinc
  func2(a++);
  //CHECK: call &func2 (conststr ptr "abc")
  func2("abc");
}

void func1(int a) {}
void func2(a, b, c) {}

int main() {
  return 0;
}
