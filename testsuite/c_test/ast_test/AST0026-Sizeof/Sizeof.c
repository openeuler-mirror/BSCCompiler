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
#include <stdio.h>

int main() {
  printf("%d,",sizeof(int));
  printf("%d,",sizeof(long));
  int a;
  printf("%d,",sizeof(a));
  struct A {
    int a;
    double b;
  };
  struct A aa;
  printf("%d,",sizeof(struct A));
  printf("%d\n",sizeof(aa));

    // complex cases
  long int l_int = -2147483647;
  char c = 'a';
  unsigned int u_int = 65535;
  unsigned char u_char = 128;
  float flt = 123.456789f;
  double db = 1.23456789;
  // long double ldb = 12.3456789123456;                //@fail case
  double ldb = 12.3456789123456;
  short int s_int = 0xa5;
  int res = c + a;
  char *p;
  printf("%lu%lu%lu%lu", sizeof(l_int), sizeof(c), sizeof(u_int), sizeof(u_char));
  printf("%lu%lu%lu%lu\n", sizeof(flt), sizeof(db), sizeof(s_int), sizeof(res));
  struct B {
    struct A a;
  } B;
  union u{
    char c;
    double d;
  } u;
  int arr[5];
  double arr2[10];
  printf("%lu%lu%lu%lu", sizeof(struct B), sizeof(p), sizeof(arr), sizeof(arr2));
  printf("%lu%lu%lu%lu%lu%lu", sizeof(8), sizeof(8.8), sizeof("arr"), sizeof(u), sizeof(u.c), sizeof(u.d));
  printf("\n%lu%lu%lu", sizeof(aa), sizeof(aa.a), sizeof(aa.b));
  return 0;
}

