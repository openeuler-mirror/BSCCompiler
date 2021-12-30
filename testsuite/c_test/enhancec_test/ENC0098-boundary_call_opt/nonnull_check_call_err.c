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

__attribute__((count_index(2, 1)))
void test3(char* p, size_t size) {
    for (int i=0; i< size; ++i) {
      printf("%p\n", p++ );
    }
}
__attribute__((count_index(2, 1)))
void test2(int *p, int len) {
    if(len >=12 ) {
      size_t count = (size_t)(unsigned int)len * (sizeof(int));
      test3((char*)p, count);
    }
}


int main() {
  return 0;
}
