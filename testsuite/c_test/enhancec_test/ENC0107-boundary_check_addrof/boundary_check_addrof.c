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
int f(int *p __attribute__((count(5)))) {
  return *p;
}

void test() {
  int idx = 10;
  int test[10] = {0};
  int j = 1;
  for (int *i = test;i < &test[idx]; i++, j++) {  // CHECK: [[# @LINE ]] warning
    *i = j;
  }

  int res = 0;
  int n = 3;
  int b[3][2] = {{1,2},{3,4},{5,6}};
  res += b[n - 1][1];  // CHECK-NOT: [[# @LINE ]] warning
  int *x = *(b + 3);  // CHECK: [[# @LINE ]] warning  
  int *y = b[n];  // CHECK: [[# @LINE ]] warning
  (void)b[n];  // CHECK: [[# @LINE ]] warning

  int *p = test + 10;  // CHECK: [[# @LINE ]] warning
  int *q = &*p;  // CHECK: [[# @LINE ]] warning

  // call
  res += f(&test[5]);  // CHECK-NOT: [[# @LINE ]] error
  // assign
  int *w __attribute__((count(5))) = &test[6];  // CHECK: [[# @LINE ]] error
}

int main() {
  test();
  return 0;
}
