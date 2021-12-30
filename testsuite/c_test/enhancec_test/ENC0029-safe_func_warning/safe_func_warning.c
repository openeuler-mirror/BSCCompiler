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

__Safe__ void func1(void);
__Unsafe__ void func2(void);


__Unsafe__ void func1(void) { // CHECK: [[# @LINE]] warning
  int a = 0;
  __Safe__ {
    a += 1;
  }
}

__Safe__ void func2(void) { // CHECK: [[# @LINE]] warning
  int a = 0;
  __Safe__ {
    a += 1;
  }
}

__Safe__ void func3(void);

void func3(void) { // CHECK-NOT: [[# @LINE]] warning
}

void func4(void);

__Safe__ void func4(void) { // CHECK-NOT: [[# @LINE]] warning
}

__attribute__((count_index(2, 1)))
__Safe__ extern int func5(char *a, int b);

int main() {
  return 0;
}
