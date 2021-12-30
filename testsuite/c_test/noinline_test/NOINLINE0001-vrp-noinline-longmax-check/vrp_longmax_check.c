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

extern void abort (void);

void testl (unsigned long l, int ok)
{
  if ((l>=1) && (l<=LONG_MAX))
  {
    if (!ok) abort ();
  }
  else {
    if (ok) abort ();
 }
}

int main ()
{
  testl (LONG_MAX+1UL, 0);
  testl (ULONG_MAX, 0);

  return 0;
}

