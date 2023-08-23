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
// CHECK: [[# FILENUM:]] "{{.*}}/IncompleteStruct.c"
// CHECK: type $FOO1{{.*}} <structincomplete {}>
struct FOO1;
void test1(struct FOO1 foo);

// CHECK: type $FOO2{{.*}} <struct {}>
// CHECK-NOT: type $FOO2{{.*}} <structincomplete
struct FOO2;
void test2(struct FOO2 foo);
struct FOO2{};

// CHECK: type $FOO3{{.*}} <struct {}>
// CHECK-NOT: type $FOO3{{.*}} <structincomplete
struct FOO3;
struct FOO3{};
void test3(struct FOO3 foo);
// CHECK: type $FOO4{{.*}} <structincomplete {}>
struct FOO4;
typedef struct FOO4 foo4;
void test5(foo4 foo);
// CHECK: type $FOO6{{.*}} <struct {}>
// CHECK-NOT: type $FOO6{{.*}} <structincomplete
// CHECK: type $FOO8{{.*}} <struct {
// CHECK-NOT: type $FOO8{{.*}} <structincomplete
// CHECK: type $FOO7{{.*}} <structincomplete {}>
// CHECK: type $FOO5{{.*}} <structincomplete {}>
struct FOO5;
extern struct FOO5 foo5;
struct FOO6{};
struct FOO6 foo6;

typedef struct FOO8 {
  struct FOO7 *p;
  int b;
} foo8;

int main() {
  return 0;
}
