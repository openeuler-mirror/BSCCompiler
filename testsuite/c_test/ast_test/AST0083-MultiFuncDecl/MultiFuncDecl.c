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
// CHECK: [[# FILENUM:]] "{{.*}}/MultiFuncDecl.c"
static void test();

void foo() {
  test();
} 

// CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
// CHECK-NEXT: func &test{{.*}} static
void test() {
  printf("test\n");
}

int main() {
  foo();
  return 0;
}
