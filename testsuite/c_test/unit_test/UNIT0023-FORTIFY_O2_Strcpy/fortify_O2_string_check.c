#define _GNU_SOURCE
#include <string.h> // 注意：此头文件必须包含
#include <stdio.h>
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
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 96, but size argument is 1000
  strcpy((char*)&x, buf4);             // 96 96
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 44, but size argument is 1000
  strcpy((char*)&x.a, buf4);           // 96 44
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 44, but size argument is 1000
  strcpy((char*)&x.a[0], buf4);        // 96 44
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.a[0].a, buf4);      // 96 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.a[0].a[0], buf4);   // 96 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 7, but size argument is 1000
  strcpy((char*)&x.a[0].a[3], buf4);   // 93 7
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2, but size argument is 1000
  strcpy((char*)&x.a[0].b, buf4);      // 86 2
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2, but size argument is 1000
  strcpy((char*)&x.a[1].b, buf4);      // 64 2
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1, but size argument is 1000
  strcpy((char*)&x.a[1].b.b, buf4);    // 63 1
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1, but size argument is 1000
  strcpy((char*)&x.a[2].b.b, buf4);    // 41 1
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.a[1].c, buf4);      // 62 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.a[1].c[0], buf4);   // 62 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 7, but size argument is 1000
  strcpy((char*)&x.a[1].c[3], buf4);   // 59 7
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 22, but size argument is 1000
  strcpy((char*)&x.b, buf4);           // 52 22
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.b.a, buf4);         // 52 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.b.a[0], buf4);      // 52 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 7, but size argument is 1000
  strcpy((char*)&x.b.a[3], buf4);      // 49 7
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2, but size argument is 1000
  strcpy((char*)&x.b.b, buf4);         // 42 2
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.b.c, buf4);         // 40 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 10, but size argument is 1000
  strcpy((char*)&x.b.c[0], buf4);      // 40 10
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 7, but size argument is 1000
  strcpy((char*)&x.b.c[3], buf4);      // 37 7
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4, but size argument is 1000
  strcpy((char*)&x.c, buf4);           // 30 4
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 4, but size argument is 1000
  strcpy((char*)&x.c[0], buf4);        // 30 4
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 3, but size argument is 1000
  strcpy((char*)&x.c[1], buf4);        // 29 3
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 1, but size argument is 1000
  strcpy((char*)&x.d, buf4);           // 26 1
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 8, but size argument is 1000
  strcpy((char*)&x.e, buf4);           // 24 8
  // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 16, but size argument is 1000
  strcpy((char*)&x.f, buf4);           // 16 16
  return 0;
}
