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

char *fpchar(void) {
  return "yes_its_possible1";
}

char *(*fpfpchar(void))(void) {
  return (fpchar);
}

char *(*(*fpfpfpchar(void))(void))(void) {
  return (fpfpchar);
}

void fpvoid(void) {
  printf("yes_its_possible2");
}

void (*fpfpvoid(void))(void) {
  return (fpvoid);
}

void (*(*fpfpfpvoid(void))(void))(void) {
  return (fpfpvoid);
}

void testv() {
  printf("yes_its_possible3");
}

int testi() {
  return 4;
}

int main() {
  char *tmp = (((fpfpfpchar())())());
  printf("%s", tmp);
  (((fpfpfpvoid())())());
  testv();
  int a = testi();
  printf("yes_its_possible%d", a);
  return 0;
}
