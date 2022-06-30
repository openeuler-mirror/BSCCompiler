/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <string.h>

#define IS_STR_EMPTY(str)	        \
do {	                                \
  __label__ empty, not_empty, exit;     \
  if (strlen(str))			\
    goto not_empty;		        \
  else					\
    goto empty;				\
				        \
  not_empty:				\
    printf("string = %s\n", str);	\
    goto exit;				\
  empty:			        \
    printf("string is empty\n");        \
  exit: ;                               \
} while(0);				\

int main()
{
  char string[20] = {'\0'};
  IS_STR_EMPTY(string);
  strcpy(string, "test");
  IS_STR_EMPTY(string);
  return 0;
}
