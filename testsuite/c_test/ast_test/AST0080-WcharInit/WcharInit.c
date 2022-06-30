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
#include <uchar.h>
#include <stddef.h>
#include <string.h>

int strcmp_ctowc( char a[], wchar_t b[] ) {
  int c = 0;

  while( a[c] == b[c] ){
      if( a[c] == '\0' || b[c] == '\0' ){
          break;
      }
      c++;
  }

  if( a[c] == '\0' && b[c] == '\0' ){
      return 0;
  } else {
      return -1;
  }
}

int main() {
  char     a [] =   "abcd";
  char     b [] = u8"abcd";
  wchar_t  c [] =  L"abcd";

  if( strcmp_ctowc(  a, c ) != 0 ) {
    abort();
  }
}
