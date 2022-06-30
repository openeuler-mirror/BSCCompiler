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

// CHECK: [[# FILENUM:]] "{{.*}}/nonnull_check_field.c"

#include <stddef.h>

int g_i = 1;
int *g_p __attribute__((nonnull)) = &g_i;

struct {
  int (*f)();
} g1;

int test1() {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: assertnonnull <&test1> (dread ptr $g1 1)
  return g1.f(); // expected-warning
}


struct {
  int (*f)() __attribute__((nonnull));
} g2;

int test2() {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: assertnonnull <&test2> (dread ptr $g2 1)
  return g2.f();
}


struct {
  int *f;
} g3;

int test3() {
  if (g3.f != NULL) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NEXT: assertnonnull <&test3> (dread ptr $g3 1)
    return *g3.f;
  }
  return 0;
}

struct {
  int *f __attribute__((nonnull));
} g4;

int test4() {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: assertnonnull <&test4> (dread ptr $g4 1)
  return *g4.f;
}

typedef struct {
  int count;
  int val;
} g5;

int get_test5(g5 *f) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: assertnonnull <&get_test5> (dread ptr %f)
  return f->count; // expected-warning {{illegal access of nullable pointer}}
}

typedef struct g6 {
  int *count __attribute__((nonnull));
  struct g6 *ptr;
} g6;

__attribute__((nonnull))
void nonnull_field(g6 *f) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: assertnonnull{{.*}}
  *(f->count);
}

__attribute__((nonnull(2, 3)))
void call_nonnull_field(g6 *f, g6 *g, g6 *h) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: callassertnonnull <&nonnull_field, 0, &call_nonnull_field> (dread ptr %f)
  nonnull_field(f); // expected-warning
  nonnull_field(g);
  nonnull_field(h);

  if (f) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NEXT: callassertnonnull <&nonnull_field, 0, &call_nonnull_field> (dread ptr %f)
    nonnull_field(f);
    if (f->count)
      // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
      // CHECK-NEXT: callassertnonnull <&nonnull_field, 0, &call_nonnull_field> (dread ptr %f)
      nonnull_field(f);
  }

  if (g->count) {
    nonnull_field(g);
  }
}

typedef struct g7 {
  int *count;
  struct g7 *ptr;
} g7;

void nullable_field(g7 *f) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: assertnonnull{{.*}}
  *(f->count); // expected-warning
  if (f != NULL && f->count) {
    *(f->count);
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK: assertnonnull{{.*}}
    *((f + 1)->count); // expected-warning
  }
  if ((f + 1) != NULL && (f + 1)->count) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK: assertnonnull{{.*}}
    *((f + 1)->count);
  }
}


int main() {
  return 0;
}
