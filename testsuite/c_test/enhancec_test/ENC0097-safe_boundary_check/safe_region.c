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

int g_arr[] = {1,2,3,4,5};
int *g_p = g_arr;

void func0(void) {
  g_p += 1;  // CHECK-NOT: [[# @LINE ]] error
  __Safe__ {
    g_p += 1;  // CHECK: [[# @LINE ]] error
  }
  g_p += 1;  // CHECK-NOT: [[# @LINE ]] error
}

#pragma SAFE OFF

void func1(void) {
  g_p += 1;  // CHECK-NOT: [[# @LINE ]] error
}

__Safe__ int func2(void);

#pragma SAFE ON

__Unsafe__ void func3(void) {
  g_p += 1;  // CHECK-NOT: [[# @LINE ]] error
}

void func4() {
  g_p += 1;  // CHECK: [[# @LINE ]] error
}

int main(void) {
  return 0;
}

__Safe__ int func2(void) {
  int a = 0;
  __Safe__ {
    g_p += 1;  // CHECK: [[# @LINE ]] error
    __Unsafe__ {
      g_p += 1;  // CHECK-NOT: [[# @LINE ]] error
      __Safe__ {
        g_p += 1;  // CHECK: [[# @LINE ]] error
      }
      g_p += 1;  // CHECK-NOT: [[# @LINE ]] error
    }
    g_p += 1;  // CHECK: [[# @LINE ]] error
  }
  g_p += 1;  // CHECK: [[# @LINE ]] error
  return g_p[2];  // CHECK: [[# @LINE ]] error
}
