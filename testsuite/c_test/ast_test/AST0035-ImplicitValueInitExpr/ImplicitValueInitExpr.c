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
  int arr[2];
  struct Str2 d;
};

int main() {
  struct Str1 myStr1 = {1};
  printf("%d:%f:%d:%d:%hd:%d\n", myStr1.a, myStr1.b, myStr1.arr[0], myStr1.arr[1], myStr1.d.c, myStr1.d.e);

  struct Str2 myStr2 = {4};
  struct Str1 myStr3 = {1, .d = myStr2};
  printf("%d:%f:%d:%d:%hd:%d\n", myStr3.a, myStr3.b, myStr3.arr[0], myStr3.arr[1], myStr3.d.c, myStr3.d.e);

  myStr2.e = 3;
  struct Str1 myStr4 = {1, 1.5, .d = myStr2, .d.c = 5 };
  printf("%d:%f:%d:%d:%hd:%d\n", myStr4.a, myStr4.b, myStr4.arr[0], myStr4.arr[1], myStr4.d.c, myStr4.d.e);

  struct Str1 myStr5 = {1, .arr = {6, 7}, .d.c = 8};
  printf("%d:%f:%d:%d:%hd:%d\n", myStr5.a, myStr5.b, myStr5.arr[0], myStr5.arr[1], myStr5.d.c, myStr5.d.e);

  return 0;
}
