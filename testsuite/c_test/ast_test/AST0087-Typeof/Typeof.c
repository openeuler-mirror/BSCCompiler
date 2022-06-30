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
#define FINDMAX(m, x, n)\
{\
  typeof(n) _n = n;           /* _n is local copy of the number of elements*/\
  if (_n > 0) {               /* in case the array is empty */\
    int _i;\
    typeof((x)[0]) * _x = x;  /* _x is local copy of pointer to the elements */\
    typeof(m) _m = _x[0];     /* _m is maximum value found */\
    for (_i=1; _i<_n; _i++)   /* Iterate through all elements */\
      if (_x[_i] > _m)\
        _m = _x[_i];\
    (m) = _m;                 /* returns the result */\
  }\
}

int main()
{
  int i[] = {69891, 71660, 71451, 71434, 72317}, im;
  float f[] = {69891., 71660., 71451., 71434., 72317.}, fm;
  double d[] = {69891., 71660., 71451., 71434., 72317.}, dm, * dp = d;
  int n = 5;
  FINDMAX(im, i, n--);
  FINDMAX(fm, f, n--);
  FINDMAX(dm, dp+=2, n--);
  printf("%d %f %f\n", im, fm, dm);
}
