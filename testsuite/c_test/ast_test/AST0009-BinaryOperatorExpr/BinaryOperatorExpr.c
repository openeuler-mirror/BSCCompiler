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

int a = 1;
struct Person{
  int age;
  char c_name;
};

int main() {
  int b = 10;
  int a = 2;
  printf("%d + %d=%d\n", a, b, a + b);
  printf("%d - %d=%d\n", a, b, a - b);
  printf("%d * %d=%d\n", a, b, a * b);
  printf("%d / %d=%d\n", a, b, a / b);
  printf("%d %% %d=%d\n", a, b, a % b);
  printf("%d << %d=%d\n", a, b, a << b);
  printf("%d >> %d=%d\n", a, b, a >> b);
  printf("%d > %d=%d\n", a, b, a > b);
  printf("%d < %d=%d\n", a, b, a < b);
  printf("%d <= %d=%d\n", a, b, a <= b);
  printf("%d >= %d=%d\n", a, b, a >= b);
  printf("%d == %d=%d\n", a, b, a == b);
  printf("%d != %d=%d\n", a, b, a != b);
  printf("%d & %d=%d\n", a, b, a & b);
  printf("%d ^ %d=%d\n", a, b, a ^ b);
  printf("%d | %d=%d\n", a, b, a | b);
  printf("%d && %d=%d\n", a, b, a && b);
  printf("%d || %d=%d\n", a, b, a || b);
  printf("%d , %d=%d\n", a, b, (a, b));

  // add more complex cases
  long int l_int = -2147483647;
  char c = 'a';
  unsigned int u_int = 65535;
  unsigned char u_char = 128;
  float flt = 123.456789f;                                                                     // @fail if no 'f' at end
  double db = 1.23456789;
  // long double ldb = 12.3456789123456;                                                                      //@fail case
  double ldb = 12.3456789123456;
  short int s_int = 0xa5;
  int res = c + a;
  printf("+:%ld%ud%.2f%f\n",(l_int + c), (u_int + u_char), (flt + db), (ldb + s_int));
  printf("-:%ld%ud%.2f%f\n",(l_int - c), (u_int - u_char), (flt - db), (ldb - s_int));
  printf("*:%ld%ud%.2f%f\n",(l_int * c), (u_int * u_char), (flt * db), (ldb * s_int));
  printf("/:%ld%ud%.2f%f\n",(l_int / c), (u_int / u_char), (flt / db), (ldb / s_int));
  // printf("%%:%ld%ud%d%d\n",(l_int % c), (u_int % u_char), ((int)flt % (int)db), ((int)ldb / (int)s_int)); //@fail case
  // printf("&:%ld%ud%d%d\n",(l_int & c), (u_int & u_char), ((int)flt & (int)db), (int)ldb & (int)s_int);    //@fail case
  // printf("&:%ld%ud%d%d\n",(l_int | c), (u_int | u_char), ((int)flt | (int)db), (int)ldb | (int)s_int);    //@fail case
  // printf("&:%ld%ud%d%d\n",(l_int ^ c), (u_int ^ u_char), ((int)flt ^ (int)db), (int)ldb ^ (int)s_int);    //@fail case

  struct Person p;
  struct Person *ptr = &p;
  p.age = 10;
  p.c_name = 'p';
  printf("%d%d", ptr->age + 10, ptr->c_name + 10);
  printf("%d%d", p.age + 10, p.c_name + 10);
  return 0;
}
