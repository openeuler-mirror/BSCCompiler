#include <stdio.h>
#include <string.h> // 注意：此头文件必须包含
struct Z {
  char a; // bit
  char b;
} z;


struct A {
  char a[10]; // char *
  struct Z b;
  char c[10];
} y, w[4];

int main() {
  char buf4[1000];
  struct B {
    struct A a[2];
    struct A b;
    char c[4];
    char d;
    double e;
    _Complex double f;
  } x;
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 96, but size argument is 1000
  memcpy(&x, buf4, 1000);             // 96 96
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 96, but size argument is 1000
  memcpy(&x.a, buf4, 1000);           // 96 44
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 96, but size argument is 1000
  memcpy(&x.a[0], buf4, 1000);        // 96 44
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 96, but size argument is 1000
  memcpy(&x.a[0].a, buf4, 1000);      // 96 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 96, but size argument is 1000
  memcpy(&x.a[0].a[0], buf4, 1000);   // 96 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 93, but size argument is 1000
  memcpy(&x.a[0].a[3], buf4, 1000);   // 93 41
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 86, but size argument is 1000
  memcpy(&x.a[0].b, buf4, 1000);      // 86 2
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 64, but size argument is 1000
  memcpy(&x.a[1].b, buf4, 1000);      // 64 2
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 63, but size argument is 1000
  memcpy(&x.a[1].b.b, buf4, 1000);    // 63 1
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 41, but size argument is 1000
  memcpy(&x.a[2].b.b, buf4, 1000);    // 41 1
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 62, but size argument is 1000
  memcpy(&x.a[1].c, buf4, 1000);      // 62 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 62, but size argument is 1000
  memcpy(&x.a[1].c[0], buf4, 1000);   // 62 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 59, but size argument is 1000
  memcpy(&x.a[1].c[3], buf4, 1000);   // 59 7
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 52, but size argument is 1000
  memcpy(&x.b, buf4, 1000);           // 52 22
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 52, but size argument is 1000
  memcpy(&x.b.a, buf4, 1000);         // 52 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 52, but size argument is 1000
  memcpy(&x.b.a[0], buf4, 1000);      // 52 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 49, but size argument is 1000
  memcpy(&x.b.a[3], buf4, 1000);      // 49 7
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 42, but size argument is 1000
  memcpy(&x.b.b, buf4, 1000);         // 42 2
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 40, but size argument is 1000
  memcpy(&x.b.c, buf4, 1000);         // 40 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 40, but size argument is 1000
  memcpy(&x.b.c[0], buf4, 1000);      // 40 10
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 37, but size argument is 1000
  memcpy(&x.b.c[3], buf4, 1000);      // 37 7
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 30, but size argument is 1000
  memcpy(&x.c, buf4, 1000);           // 30 4
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 30, but size argument is 1000
  memcpy(&x.c[0], buf4, 1000);        // 30 4
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 29, but size argument is 1000
  memcpy(&x.c[1], buf4, 1000);        // 29 3
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 26, but size argument is 1000
  memcpy(&x.d, buf4, 1000);           // 26 1
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 24, but size argument is 1000
  memcpy(&x.e, buf4, 1000);           // 24 8
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 16, but size argument is 1000
  memcpy(&x.f, buf4, 1000);           // 16 16
  struct A a[10];
  // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 44, but size argument is 1000
  memcpy(&a[8], buf4, 1000);
  return 0;
}
