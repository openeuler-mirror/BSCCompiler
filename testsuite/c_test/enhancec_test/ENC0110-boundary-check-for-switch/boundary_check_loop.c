/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <stdlib.h>
#include <string.h>

int stab_xcoff_builtin_type (int typenum)
{
  const char *name;
  if (typenum >= 0 || typenum < -3)
    {
      return 0;
    }
  switch (-typenum)
    {
    case 1:
      name = "int";
      break;
    case 2:
      name = "char";
    case 3:
      name = "short";
    }
  return name[0];
}

extern void abort (void);
int main()
{
  return 0;
}
