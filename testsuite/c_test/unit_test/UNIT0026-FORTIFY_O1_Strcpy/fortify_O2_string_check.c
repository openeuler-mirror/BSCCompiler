#include <stdio.h>
#include <string.h> // 注意：此头文件必须包含
struct C {
  short a[10];
} c;

struct Z {
  int a[10]; // bit
  struct C b[5];
} z;


struct A {
  int a[10]; // char *
  struct Z b[10];
  short c[10];
} y, w[4];

int main() {
  char buf4[10000];
  struct B {
    struct A a[2];
    struct A b;
    int c[4];
    char d;
    double e;
    _Complex double f;
  } x;
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2374, but size argument is 10000
  strcpy((char*)&x.a[1].b[3].b[4].a[5], buf4);
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2384, but size argument is 10000
  strcpy((char*)&x.a[1].b[3].b[4].a, buf4);
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2424, but size argument is 10000
  strcpy((char*)&x.a[1].b[3].b[2], buf4);
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2464, but size argument is 10000
  strcpy((char*)&x.a[1].b[3].b, buf4);
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2484, but size argument is 10000
  strcpy((char*)&x.a[1].b[3].a[5], buf4);
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2504, but size argument is 10000
  strcpy((char*)&x.a[1].b[3].a, buf4);
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4424, but size argument is 10000
  strcpy((char*)&x, buf4);             // 96 96
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4424, but size argument is 10000
  strcpy((char*)&x.a, buf4);           // 96 44
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4424, but size argument is 10000
  strcpy((char*)&x.a[0], buf4);        // 96 44
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4424, but size argument is 10000
  strcpy((char*)&x.a[0].a, buf4);      // 96 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4424, but size argument is 10000
  strcpy((char*)&x.a[0].a[0], buf4);   // 96 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4412, but size argument is 10000
  strcpy((char*)&x.a[0].a[3], buf4);   // 93 41
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4384, but size argument is 10000
  strcpy((char*)&x.a[0].b, buf4);      // 86 2
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2924, but size argument is 10000
  strcpy((char*)&x.a[1].b, buf4);      // 64 2
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2884, but size argument is 10000
  strcpy((char*)&x.a[1].b[0].b, buf4);    // 63 1
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1144, but size argument is 10000
  strcpy((char*)&x.a[2].b[2].b, buf4);    // 41 1
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1524, but size argument is 10000
  strcpy((char*)&x.a[1].c, buf4);      // 62 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1524, but size argument is 10000
  strcpy((char*)&x.a[1].c[0], buf4);   // 62 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1518, but size argument is 10000
  strcpy((char*)&x.a[1].c[3], buf4);   // 59 7
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1504, but size argument is 10000
  strcpy((char*)&x.b, buf4);           // 52 22
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1504, but size argument is 10000
  strcpy((char*)&x.b.a, buf4);         // 52 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1504, but size argument is 10000
  strcpy((char*)&x.b.a[0], buf4);      // 52 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1492, but size argument is 10000
  strcpy((char*)&x.b.a[3], buf4);      // 49 7
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1464, but size argument is 10000
  strcpy((char*)&x.b.b, buf4);         // 42 2
  return 0;
}
