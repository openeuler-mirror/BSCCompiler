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
#include <stddef.h>

struct Str2 {
  short c;
  int e;
};

struct Str1 {
  int a;
  double b;
  char ch[2];
  struct Str2 d;
};

int main() {
  // printf("%c", myStr1.ch[0]) not support temporarily
  struct Str1 myStr1 = {1, 1.5, {'a', 'b'}, {2, 3}};
  printf("%d:%f:%hd:%d\n", myStr1.a, myStr1.b, myStr1.d.c, myStr1.d.e);

  struct Str2 myStr2 = {4, 5};
  struct Str1 myStr3 = {1, 1.5, {'a', 'b'}, .d = myStr2};
  printf("%d:%f:%hd:%d\n", myStr3.a, myStr3.b, myStr3.d.c, myStr3.d.e);

  struct Str1 myStr4 = {1, 1.5, {'a', 'b'}, .d = myStr2, .d.c = 6, .d.e = 7};
  printf("%d:%f:%hd:%d\n", myStr4.a, myStr4.b, myStr4.d.c, myStr4.d.e);

  struct Str1 myStr5 = {1, 1.5, {'a', 'b'}, .d = myStr2, .d.c = 8};
  printf("%d:%f:%hd:%d\n", myStr5.a, myStr5.b, myStr5.d.c, myStr5.d.e);

  struct Str2 myStr6 = (struct Str2){9, 10};
  printf("%hd:%d\n", myStr6.c, myStr6.e);

  size_t lenA = offsetof(struct Str1, a);
  printf("%ld\n", lenA);

  size_t lenB = offsetof(struct Str1, b);
  printf("%ld\n", lenB);

  size_t lenCh = offsetof(struct Str1, ch);
  printf("%ld\n", lenCh);

  size_t lenD = offsetof(struct Str1, d);
  printf("%ld\n", lenD);

  return 0;
}
