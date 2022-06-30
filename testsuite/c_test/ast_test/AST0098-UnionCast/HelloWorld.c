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

struct tree_string
{
  char str[1];
};

union tree_node
{
  struct tree_string string;
};

char *Foo (char *str)
{
  char *str1 = ((union {const char * _q; char * _nq;}) str)._nq;
  return str1;
}

char *Foo1 (union tree_node * num_string)
{
  char *str = ((union {const char * _q; char * _nq;})
               ((const char *)(({ __typeof (num_string) const __t
                                     = num_string;  __t; })
                               ->string.str)))._nq;
  return str;
}

int main() {
  char *i = Foo("123");
  union tree_node node = {'4'};
  char *j = Foo1(&node);
  printf("%c,%c\n", i[2], j[0]);
  return 0;
}
