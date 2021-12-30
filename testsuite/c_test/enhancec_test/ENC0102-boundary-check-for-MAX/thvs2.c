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
#include <limits.h>

/* I'm doing something trying to screw up loop optimizations
 * and return 1 when I know everything is OK
 */
#define I 2
#define K 3
#define J 4
int a[I][K] = { { 0, 1, 2}, {3, 4, 5} };
int b[K][J] = { { 0, 1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11} };
int c[I][J];

int
test(void)
{
  int i;
  int j;
  int k;
  int s = 0;

  for (i=INT_MAX-I; i<INT_MAX; i++) {
    printf("i=%d, INT_MAX=%d: ", i, INT_MAX);
    for (j=INT_MIN; j<=J-1+INT_MIN; j++) {
      printf("j=%d, INT_MIN=%d: ", j, INT_MIN);
      printf("i:[%d] j:[%d]", i-INT_MAX+I, j-INT_MIN);
      for (k=0; k<K; k++) {
        c[i-INT_MAX+I][j-INT_MIN]
          += a[i-INT_MAX+I][k]
            *b[k][j-INT_MIN];
      }
      printf("\n");
    }
  }
  for (i=0; i<I; i++) {
    for (j=0; j<J; j++) {
      s += c[i][j];
    }
  }
  return s;
}

int main()
{
   if (test() != 394) {
     printf("error\n");
     return -1;
   }
   return 0;
}
