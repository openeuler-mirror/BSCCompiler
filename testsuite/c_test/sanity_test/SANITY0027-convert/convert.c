/*
 * Copyright (C) [2021] Futurewei Technologies, Inc. All rights reverved.
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

signed char c = 0xff;
unsigned char uc = 0xff;
short s;
int i;
long long l;

int main()
{
  s = c;
  i = s;
  l = i;
  //printf("signed = %ld ", l);
  if (l != -1) {
    abort();
  }

  s = uc;
  i = s;
  l = i;
  //printf("unsigned = %ld\n", l);
  if (l != 255) {
    abort();
  }

  return 0;
}
