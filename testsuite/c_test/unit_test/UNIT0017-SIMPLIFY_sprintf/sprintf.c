/*
 * Copyright (c) [2022-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include <stdlib.h>
#include <stdio.h>
extern void abort();
int test(char *s) {
  return sprintf_s(s, 4, "123"); // replace with memcpy(s, "123", 3)
}

int test1(char *s) {
  return snprintf_s(s, 4, 5, "123"); // replace with memcpy(s, "123", 3)
}
int test2(char *s) {
  return vsnprintf_s(s, 4, 5, "%s", "123"); // replace with memcpy(s, "123", 3)
}

int test3(char *s) {
  return sprintf(s, "123"); // replace with memcpy(s, "123", 3)
}

int test4(char *s) {
  char *s1;
  return sprintf(s, s1); // no need replace
}

int test5(char *s) {
  char *s1;
  return sprintf_s(s, 4, "1234"); // truncate, no need replace, return -1
}

int test6(char *s) {
  return sprintf_s(s, 0, "123"); // dstMax = 0, no need replace, return -1
}

int test7(char *s) {
  return snprintf_s(s, 1, 0, "123"); // count = 0, no need replace, return -1
}

int test8(char *s) {
  int n = 1;
  return snprintf_s(s, n, 1, "123"); // unknow dstMax, no need replace
}

int test9(char *s) {
  return sprintf_s(s, 5, "%s", "123", "123"); // too many parameters, no need replace
}

int test10(char *s) {
  return snprintf_s(s, 5, 4,  "%s", "123", "123"); // too many parameters, no need replace
}

int test11(char *s) {
  return snprintf_s(s, 5, 4,  "string %s", "123"); // fmt string is not only "%s", no need replace
}
int test12(char *s) {
  return sprintf_s(s, 5, "string %u", 1); // fmt string is not only "%s", no need replace
}

int main() {
  char s[10];
  int a = test(s);
  if (a != 3 || strcmp(s, "123") != 0){
    abort();
  }
  a = test1(s);
  if (a != 3 || strcmp(s, "123") != 0) {
    abort();
  }
  a = test2(s);
  if (a != 3 || strcmp(s, "123") != 0) {
    abort();
  }
  a = test3(s);
  if (a != 3 || strcmp(s, "123") != 0) {
    abort();
  }
  a = test5(s);
  if (a != -2) {
    abort();
  }
  a = test6(s);
  if (a != -1) {
    abort();
  }
  a = test7(s);
  if (a != -1) {
    abort();
  }
  return 0;
}

