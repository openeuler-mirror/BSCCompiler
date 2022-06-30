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
#include <stdlib.h>
__attribute__((returns_count("len")))
int *getBoundaryPtr(int len, int len1) {
  int* ret = (int*) malloc(len1 * sizeof(int));
  return ret; // CHECK: [[# @LINE ]] warning: can't prove return value's bounds match the function declaration for getBoundaryPtr
}


int main() {
  int *pa1 = getBoundaryPtr(4, 3) + 1;
  printf("%d\n", pa1[2]);

  return 0;
}
