/*
 * Copyright (C) [2020] Futurewei Technologies, Inc. All rights reverved.
 *
 * Licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

//int main(int argc, char *argv[])
int main()
{
  int a[10], b[10], c[10], i;

  for (i = 0; i < 10; i++) {
    b[i] = i << 2;
    c[i] = i << (b[i] % (sizeof(int)*CHAR_BIT));
  }

  for (i = 0; i < 10; i++) {
    a[i] = b[i] + c[9-i];
  }

  int sum = 0;
  for (i = 0; i < 10; i++) {
    sum += a[i];
  }

  if (sum != 1985229660) {
    abort();
  }

  return 0;
}
