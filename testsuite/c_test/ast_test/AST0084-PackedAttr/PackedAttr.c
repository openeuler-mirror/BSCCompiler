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

// CHECK: [[# FILENUM:]] "{{.*}}/PackedAttr.c"
// CHECK: type {{.*}} <struct pack(2)
#pragma pack(push, 2)
typedef struct {
  char c1;
  int i;
  char c2;
} FOO1;
#pragma pack(pop)
// CHECK: type {{.*}} <struct pack(4)
#pragma pack(push, 1)
#pragma pack(push, 4)
typedef struct {
  char c1;
  int i;
  char c2;
} FOO2;
#pragma pack(pop)
#pragma pack(pop)
// CHECK: type {{.*}} <struct pack(1)
typedef struct {
  char c1;
  int i;
  char c2;
}__attribute__ ((packed)) FOO3;
// CHECK: type $foo1{{.*}} <struct pack(1)
struct foo1 {
  char c1;
  int i;
  char c2;
} __attribute__ ((packed));
// CHECK: type $foo2{{.*}} <struct pack(1)
#pragma pack(push, 2)
struct foo2 {
  char c1;
  int i;
  char c2;
} __attribute__ ((packed));
#pragma pack(pop)
struct foo3 {
  char c1;
  // CHECK: i {{.*}} pack(1)
  int i  __attribute__ ((packed));
  char c2;
};
// CHECK: type $foo4{{.*}} <struct pack(2)
#pragma pack(push, 2)
struct foo4 {
  char c1;
  // CHECK: i {{.*}} pack(1)
  int i  __attribute__ ((packed));
  char c2;
};
#pragma pack(pop)
// CHECK: type $foo5{{.*}} <struct pack(1)
struct foo5 {
  char c1;
  // CHECK: i {{.*}} pack(1)
  int i  __attribute__ ((packed));
  char c2;
} __attribute__ ((packed));

int main() {
  return 0;
}
