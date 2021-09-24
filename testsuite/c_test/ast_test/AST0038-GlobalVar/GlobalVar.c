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
// double c = 3.14159;                                                              // fail case
// long double d = 1.11111;                                                         // fail case
// float arr3[] = {1.0, -1.0, 0 , 0};                                               // fail case
// float arr4[] = {1.0f, -1.0, 0 , 0};                                              // fail case
// float arr4[] = {1.0f, -1.0f};                                                    // fail case, wrong value
// struct A A = {10, 10.1, 10.2, 'a'};                                              // fail case
int a = 1;
float b = 1.1;
struct A {
  int Aa;
  float Ab;
  double Ac;
  char Ad;
} A;                                                                                // A = {10, 10.1, 10.2, 'a'} fail

struct A *pA = &A;
int arr[] = {1, 2, 3};
float arr2[3] = {0.0};
float arr3[] = {1.0, -1.0};
char arr4[] = {'a','c'};
float f = (1.17549435e-38f <= 1.1) ? 1.1 : 9.9;
struct B{
  int Barr[2];
  float Barr2[2];
  double Barr3[2];
  char Barr4[2];
} B;
struct B *pB = &B;
int lenA = sizeof(A) + 1;
enum DAY{
  MON=1, TUE, WED, THU, FRI, SAT, SUN
} day = WED;
union data{
    int n;
    char ch;
    double f;
} data;
// const struct C {                                                                 // fail case
//   int a;
//   char c;
//   double d;
// } C = {1, 'a', 2.222};
int main() {
  A.Aa = 10;
  A.Ab = 10.1;
  A.Ac = 10.2;
  A.Ad = 'a';
  B.Barr[0] = 20;
  B.Barr[1] = 22;
  B.Barr2[0] = 20.1;
  B.Barr2[1] = 22.1;
  B.Barr3[0] = 20.131;
  B.Barr3[1] = 22.131;
  B.Barr4[0] = 'b';
  B.Barr4[1] = 'c';
  data.n = 4;
  data.ch ='s';
  data.f = 1.234;
  printf("%d\n", a);
  printf("%f\n", b);
  printf("%f\n", A.Ac);
  printf("%d%d%d\n", arr[0], arr[1], arr[2]);
  printf("%f%f%f\n", arr2[0], arr2[1], arr2[2]);
  printf("%f%f\n", arr3[0], arr3[1]);
  printf("%c%c\n", arr4[0], arr4[1]);
  printf("%f\n", f);
  printf("%d%d%d\n", *arr, *(arr + 1), *(arr + 2));
  printf("%f%f%f\n", *arr2, *(arr2 + 1), *(arr2 + 2));
  printf("%f%f\n", *arr3, *(arr2 + 1));
  printf("%c%c\n", *arr4, *(arr4 + 1));
  printf("%d%d%d", lenA, sizeof(pA), sizeof(pB));
  printf("%d%d\n", day, sizeof(day));
  printf("%d%c%f\n",data.n, data.ch, data.f);
  // printf("%c%c%c", A.Ad, B.Barr4[0], B.Barr4[1]);                                  // fail case
  // printf("%d\n", A.Aa);                                                            // fail case, wrong value
  // printf("%f\n", A.Ab);                                                            // fail case, wrong value
  // printf("%d%f%f\n", pA->Aa, pA->Ab, pA->Ac);                                      // fail case
  // printf("%d%d\n", pB->Barr[0], pB->Barr[1]);                                      // fail case
  // printf("%f%f%f%f\n", pB->Barr2[0], pB->Barr2[1], pB->Barr3[0], pB->Barr3[1]);    // fail case
  // printf("%f%f\n", arr4[0], arr4[1]);
  return 0;
}
