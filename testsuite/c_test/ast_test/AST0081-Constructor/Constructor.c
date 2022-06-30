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
#include <stdint.h>


void __attribute__((constructor(103))) z_1() {
  printf("constructor(103)\n");
}

void __attribute__((constructor(102))) z_2() {
  printf("constructor(102)\n");
}

void __attribute__((constructor())) z_3() {
  printf("constructor\n");
}

void __attribute__((destructor(102))) z_4() {
  printf("destructor(102)\n");
}

void __attribute__((constructor(105))) __attribute__((destructor(105))) z_5() {
  printf("constructor/destructor(105)\n");
}

int main() {
  return 0;
}
