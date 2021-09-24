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

struct Person1 {
  char name;
  int age;
};

struct Person2 {
  int age;
  char name;
};

int main() {
  struct Person1 p1;
  struct Person2 p2;
  p1.name = 'a';
  p1.age = 100;
  p2.name = 'a';
  p2.age = 100;
  printf("%d%d\n", p1.age, p1.name);
  printf("%d%c\n", p1.age, p1.name);
  printf("%d%d\n", p2.age, p2.name);
  printf("%d%c\n", p2.age, p2.name);
  return 0;
}
