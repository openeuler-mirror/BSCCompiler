/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <stdarg.h>

typedef struct demo1 {
  int a;
  short b;
} demo1;

typedef struct demo2 {
  float a;
  float b;
  float c;
} demo2;  // HFA 

typedef struct demo3 {
  double a;
  double b;
  double c;
} demo3;  // copyed mem

typedef struct demo4 {
  float a;
  float arr[3];
} demo4;  // HFA nested array

void func(int a, ...) { // reg
  va_list vl;
  printf("a: %d\n",a);
  va_start(vl, a);
  double b = va_arg(vl, double);
  printf("b: %lf\n", b);
  long c = va_arg(vl, long);
  printf("c: %ld\n", c);
  char *d = va_arg(vl, char *);
  printf("d: %s\n", d);
  demo1 e = va_arg(vl, demo1);
  printf("e.a: %d\n", e.a);
  printf("e.b: %d\n", e.b);
  demo2 f = va_arg(vl, demo2);
  printf("f.a: %lf\n", f.a);
  printf("f.b: %lf\n", f.b);
  printf("f.c: %lf\n", f.c);
  demo3 g = va_arg(vl, demo3);
  printf("g.a: %lf\n", g.a);
  printf("g.b: %lf\n", g.b);
  printf("g.c: %lf\n", g.c);
  demo4 h = va_arg(vl, demo4);
  printf("g.a: %lf\n", h.a);
  printf("g.a: %lf\n", h.arr[2]);
  va_end(vl);
}

void func1(int a, ...) { // stack
  va_list vl1;
  printf("a: %d\n",a);
  va_start(vl1, a);
 
  for (int i = 1; i <= 8; i++) {
    int ii = va_arg(vl1, int);
    printf("ii%d: %d\n", i, ii);
  }

  for (int i = 1; i <= 8; i++) {
    double dd = va_arg(vl1, double);
    printf("dd%d: %lf\n", i, dd);
  }

  double b = va_arg(vl1, double);
  printf("b: %lf\n", b);
  long c = va_arg(vl1, long);
  printf("c: %ld\n", c);
  char *d = va_arg(vl1, char *);
  printf("d: %s\n", d);
  demo1 e = va_arg(vl1, demo1);
  printf("e.a: %d\n", e.a);
  printf("e.b: %d\n", e.b);
  demo2 f = va_arg(vl1, demo2);
  printf("f.a: %lf\n", f.a);
  printf("f.b: %lf\n", f.b);
  printf("f.c: %lf\n", f.c);
  demo3 g = va_arg(vl1, demo3);
  printf("g.a: %lf\n", g.a);
  printf("g.b: %lf\n", g.b);
  printf("g.c: %lf\n", g.c);
  demo4 h = va_arg(vl1, demo4);
  printf("g.a: %lf\n", h.a);
  printf("g.a: %lf\n", h.arr[2]);
  va_end(vl1);
}

int main() {
  int a = 20;       // int 
  double b = 30.0;  // double
  long c = 123L;
  char *d = "abc";  // ptr
  demo1 e;          // struct
  e.a = 100;
  e.b = 101;
  demo2 f;
  f.a = 1.1f;
  f.b = 1.2f;
  f.c = 1.3f;
  demo3 g;
  g.a = 3.14;
  g.b = 1.234;
  g.c = 1.0;
  demo4 h;
  h.a = 2.22;
  h.arr[2] = 3.33;
  func(a, b, c, d, e, f, g, h);
  func1(a, 1, 2, 3, 4, 5, 6, 7 ,8, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6 ,7.7, 8.8, b, c, d, e, f, g, h);
  return 0;
}
