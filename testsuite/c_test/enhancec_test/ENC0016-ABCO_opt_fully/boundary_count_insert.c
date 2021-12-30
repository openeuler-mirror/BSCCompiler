/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <limits.h>

__attribute__((count("len", 2)))
void test1(int len, int *p) {
  if (len != 5) {
    return;
  }
  for (int i = len - 1; i >= 0; --i) {
    printf("%d\n", p[i]);
  }

  for (int i = len - 1; i > -1; --i) {
    printf("%d\n", p[i]);
  }

  for (int i = 0; i != len; ++i) {
    printf("%d\n", p[i]);
  }

  for (int i = len - 1; i != 0; --i) {
    printf("%d\n", p[i]);
  }

  for (int i = len - 1; i != -1; --i) {
    printf("%d\n", p[i]);
  }

  for (int i = 0; i < len; ++i) {
    printf("%d\n", p[i]);
  }

  for (int i = 0; i <= len - 1; ++i) {
    printf("%d\n", p[i]);
  }
}

__attribute__((count("len", 2)))
int test2(int len, int *p) {
  int res = 0;
  if (len != 5) {
    return res;
  }
  int x = 0;
  do {
    res += p[x];
    x++;
  } while (x < len);
  return res;
}

__attribute__((count("len", 2)))
int test4(int len, int *p) {
  if (len != 1) {
    return 0;
  }
  int res = 0;
  for (int i = 0; i <= 10; i++) {
    for (int j = 4; j < i - 5; j++) {
      res += p[j - 4];
    }
  }
  return res;
}

__attribute__((count("len", 2)))
int test5(int len, int *p) {
  if (len != 1) {
    return 0;
  }
  int res = 0;
  for (int i = 0; i <= 10; i++) {
    for (int j = 4; ; j++) {
      if (j > 20) {
        break;
      }
      if (j > 9) {
        break;
      }
      if (j > 4) {
        break;
      }

      res += p[j - 4];
    }
  }
  return res;
}

__attribute__((count("len", 2)))
int test6(int len, int *p) {
  if (len != 1) {
    return 0;
  }
  int res = 0;
  for (int i = 0; i <= 10; i++) {
    for (int j = 4; ; j++) {
      if (j > 9) {
        break;
      }
      if (j > 4) {
        printf("%d %d\n", i , j);
      }
      if (j > 4) {
        break;
      }

      res += p[j - 4];
    }
  }
  return res;
}

__attribute__((count("len", 2)))
int test7(int len, int *p) {
  if (len != 2) {
    return 0;
  }
  int res = 0;
  for (int i = 0; i <= 10; i++) {
    for (int j = 4; j < 6; j++) {
      switch (j) {
        case 4:
          res += p[j - 4];
          break;
        case 5:
          res += p[j - 4];
          break;
        default:
          break;
      }
    }
  }
  return res;
}

int main() {
  int a[5] = {1, 2, 3, 4, 5};
  int d[1] = {1};
  int e[2] = {1, 2};
  int i = 1;
  int *p = &a;
  test1(5, p);
  p = &a;
  test2(5, p);
  p = &d;
  test4(1, p);
  test5(1, p);
  test6(1, p);
  p = &e;
  test7(2, p);
  return 0;
}
