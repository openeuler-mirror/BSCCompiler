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
struct A {
  int a;
  int b;
};

struct B {
  int c;
  int d;
  struct A singleA;
  int inB[2];
};

struct C {
    char i[20];
    char j[2][20];
    unsigned int k;
};

typedef struct D {
    char a[20];
    char b[4][20];
    unsigned int c;
    char d[1][20];
    struct C info;
} dd;

dd g_d[5] = {
    {"abc", {"abcd"}, 1, {"abc"}, {"12", {"34"}, 1}},  // b[4][20] and d[1][20] filled by an element
		{"def", {"efgh", "igkl"}, 2, {"abc"}, {"56", {"78", "910"}, 2}},
    {"", {""}, 3, {""}, {"",{""}, 3}},
};


int main() {
  struct B s1;
  s1.inB[1] = 15;
  printf("%d\n", s1.inB[1]);

  struct A saa[4] = {{1,2},{3,4},{5,6},{7,8}};
  printf("%d\n",saa[0].a);
  printf("%d\n",saa[1].b);
  saa[0].a = 10;
  printf("%d\n",saa[0].a);
  saa[1].b = 20;
  saa[0].a = 30;
  printf("%d\n",saa[0].a);
  printf("%d\n",saa[1].b);
  printf("%d\n",saa[2].a);
  printf("%d\n",saa[2].b);
  printf("%d\n",saa[3].a);
  printf("%d\n",saa[3].b);

  printf("g_d[0].a=%s, g_d[1].a=%s, g_d[2].a=%s\n",g_d[0].a, g_d[1].a, g_d[2].a);
  printf("g_d[0].b[0]=%s, g_d[1].b[0]=%s, g_d[2].b[0]=%s\n",g_d[0].b[0], g_d[1].b[0], g_d[2].b[0]);
  printf("g_d[0].b[1]=%s, g_d[1].b[1]=%s,  g_d[2].b[1]=%s\n",g_d[0].b[1], g_d[1].b[1], g_d[2].b[1]);
  printf("g_d[0].c=%d, g_d[1].c=%d, g_dd[2].c=%d\n",g_d[0].c, g_d[1].c, g_d[2].c);
  printf("g_d[0].d[0]=%s, g_d[1].d[0]=%s, g_d[2].d[0]=%s\n",g_d[0].d[0], g_d[1].d[0], g_d[2].d[0]);
  printf("g_d[0].info.j[0]=%s, g_d[1].info.j[0]=%s, g_d[2].info.j[0]=%s\n",g_d[0].info.j[0], g_d[1].info.j[0], g_d[2].info.j[0]);
  printf("g_d[0].info.j[1]=%s, g_d[1].info.j[1]=%s, g_d[2].info.j[1]=%s\n", g_d[0].info.j[1], g_d[1].info.j[1], g_d[2].info.j[1]);

  dd d[5] = {
    {"abc", {"abcd"}, 1, {"abc"}, {"12", {"34"}, 1}},  // b[4][20] and d[1][20] filled by an element
    {"def", {"efgh", "igkl"}, 2, {"abc"}, {"56", {"78", "910"}, 2}},
    {"", {""}, 3, {""}, {"",{""}, 3}},
  };

  printf("d[0].a=%s, d[1].a=%s, d[2].a=%s\n",d[0].a, d[1].a, d[2].a);
  printf("d[0].b[0]=%s, d[1].b[0]=%s, d[2].b[0]=%s\n",d[0].b[0], d[1].b[0], d[2].b[0]);
  printf("d[0].b[1]=%s, d[1].b[1]=%s, d[2].b[1]=%s\n",d[0].b[1], d[1].b[1], d[2].b[1]);
  printf("d[0].c=%d, d[1].c=%d, d[2].c=%d\n",d[0].c, d[1].c, d[2].c);
  printf("d[0].d[0]=%s, d[1].d[0]=%s, d[2].d[0]=%s\n",d[0].d[0], d[1].d[0], d[2].d[0]);
  printf("d[0].info.j[0]=%s, d[1].info.j[0]=%s, d[2].info.j[0]=%s\n",d[0].info.j[0], d[1].info.j[0], d[2].info.j[0]);
  printf("d[0].info.j[1]=%s, d[1].info.j[1]=%s, d[2].info.j[1]=%s\n", d[0].info.j[1], d[1].info.j[1], d[2].info.j[1]);
  return 0;
}
