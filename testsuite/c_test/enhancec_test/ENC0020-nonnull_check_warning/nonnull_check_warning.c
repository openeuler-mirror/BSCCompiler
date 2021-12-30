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
#include <stddef.h>

// ipa will insert nonnull
void deref_nullable(int *x) {
  int a = *x; // CHECK-NOT: [[# @LINE ]] warning
  int *y = x;
  a = *y;
}

__attribute__((nonnull))
void deref_nonnull(int *x) {
  int a = *x;
  int *y = x;
  a = *y; // CHECK-NOT: [[# @LINE ]] warning
}

void deref_nullable_if_not_null(int *x, int y) {
  if (x != NULL) {
    int a = *x; // CHECK-NOT: [[# @LINE ]] warning
    int *y = x;
    a = *y; // CHECK-NOT: [[# @LINE ]] warning
  } else {
  }
  int a = *x; // CHECK: [[# @LINE ]] warning

  if (x != NULL && y > 0) {
    int a = *x + y;  // CHECK-NOT: [[# @LINE ]] warning
  }
}

void deref_nullable_if_return(int *x) {
  if (!x)
    return;
  int a = *x; // CHECK-NOT: [[# @LINE ]] warning
}

void deref_nullable_if_null(int *x) {
  if (x == NULL) {
  } else {
    int a = *x; // CHECK-NOT: [[# @LINE ]] warning
    int *y = x;
    a = *y; // CHECK-NOT: [[# @LINE ]] warning
  }
  int a = *x; // CHECK: [[# @LINE ]] warning
}

void deref_nullable_if(int *x) {
  if (x) {
    int a = *x; // CHECK-NOT: [[# @LINE ]] warning
    int *y = x;
    a = *y; // CHECK-NOT: [[# @LINE ]] warning
  } else {
  }
}

void deref_nullable_if_not(int *x) {
  if (!x) {
  } else {
    int a = *x; // CHECK-NOT: [[# @LINE ]] warning
    int *y = x;
    a = *y; // CHECK-NOT: [[# @LINE ]] warning
  }
}

void deref_nullable_ternary(int *x, int *y __attribute__((nonnull)), int z) {
  int a = x ? *x : 0; // CHECK-NOT: [[# @LINE ]] warning
  int b = 0;
  a = (x != NULL) ? *x : 0; // CHECK-NOT: [[# @LINE ]] warning
  a = (x == NULL) ? 0 : *x; // CHECK-NOT: [[# @LINE ]] warning
  a = *(z ? y : y); // CHECK-NOT: [[# @LINE ]] warning
  b = *(z ? x : y); // CHECK: [[# @LINE ]] warning
  a = *(x ? x : y); // CHECK-NOT:[[# @LINE ]] warning
  b = *(x ? y : x); // CHECK: [[# @LINE ]] warning
}

void deref_nullable_shortcut(int *x) {
  if (x && *x > 0) // CHECK-NOT: [[# @LINE ]] warning
    return;
}

int null_check_or(int *a, int *b) {
  if (a == NULL || b == NULL) {
    return 0;
  } else {
    return *a + *b; // CHECK-NOT: [[# @LINE ]] warning
  }
}

int null_check_or_error(int *a, int b) {
  if (a != NULL || b > 0) {
    return *a + b; // CHECK: [[# @LINE ]] warning
  }
  return b;
}

int null_check_and(int *a, int *b) {
  if (a != NULL && b != NULL) {
    return *a + *b; // CHECK-NOT: [[# @LINE ]] warning
  } else {
    return 0;
  }
}

int null_check_and_error(int *a, int *b) {
  if (a == NULL && b == NULL) {
    return 0;
  } else {
    return *a + *b; // CHECK: [[# @LINE ]] warning
  }
}

typedef struct foo {
  int *count __attribute__((nonnull));
  struct foo *ptr;
} foo;

void deref_nonnull_field(foo *f __attribute__((nonnull))) {
  int a = *(f->count); // CHECK-NOT: [[# @LINE ]] warning
}

void deref_nullable_field_field(foo *f __attribute__((nonnull))) {
  int a = *(f->ptr->count); // CHECK: [[# @LINE ]] warning
}

// ipa will insert nonnull
void access_nullable(foo *f) {
  int *a = f->count; // CHECK-NOT: [[# @LINE ]] warning
}

__attribute__((nonnull))
void access_nonnull(foo *f) {
  int *a = f->count; // CHECK-NOT: [[# @LINE ]] warning
}

// ipa will insert nonnull
void array_indexing_nullable(char *in) {
  char c = in[0]; // CHECK-NOT: [[# @LINE ]] warning
}

void array_indexing_nonnull(char *in __attribute__((nonnull))) {
  char *data = "abcde";
  char c = in[0]; // CHECK-NOT: [[# @LINE ]] warning
  c = data[0]; // CHECK-NOT: [[# @LINE ]] warning
}

void func_ptr_nullable(void (*funcPtr)(void)) {
  funcPtr(); // CHECK: [[# @LINE ]] warning
}


__attribute__((nonnull))
void func_ptr_nonnull(void (*funcPtr)(void)) {
  funcPtr(); // CHECK-NOT: [[# @LINE ]] warning
}

struct List {
  struct List *next;
  int value;
};

unsigned int access_loop_check(struct List *l) {
  unsigned int length = 0;
  for (; l != NULL; l = l->next, length++) // CHECK-NOT: [[# @LINE ]] warning
    ;
  return length;
}

struct {
  int (*f)();
} g1;

int member_call_nullable() {
  return g1.f(); // CHECK: [[# @LINE ]] warning
}

struct {
  int (*f)() __attribute__((nonnull));
} g2;

int member_call_nonnull() {
  return g2.f();
}

int multi_if(int *a, int *b) {
  if (!a)
    return 0;
  if (!b)
    return 1;
  return *a + *b; // CHECK-NOT: [[# @LINE ]] warning
}

int nested_if(int *a, int *b) {
  if (a != 0) {
    if (b != 0) {
      *a = *b; // CHECK-NOT: [[# @LINE ]] warning
      return 0;
    }
    *a = 1; // CHECK-NOT: [[# @LINE ]] warning
    return 1;
  }
  return 2;
}


int ptr_ptr(int *p, int *q) {
  int **ptr_p = &p;
  if (p != NULL) {
    *ptr_p = q;
    return *p; // CHECK: [[# @LINE ]] warning
  }
  return 0;
}

int nonnull_ptr_ptr(int *p, int *q __attribute__((nonnull))) {
  int **ptr_p = &p;
  if (p != NULL) {
    *ptr_p = q;
    return *p; // CHECK-NOT: [[# @LINE ]] warning
  }
  return 0;
}

int *ptr_n_n(int **p) {
  if (p == NULL)
    return NULL;
  return *p; // CHECK-NOT: [[# @LINE ]] warning
}

int ptr_n_nn(int **p) {
  if (p == NULL)
    return 0;
  if (*p == NULL)
    return 0;
  return **p; // CHECK-NOT: [[# @LINE ]] warning
}


int *nonnull_ptr_n_n(int **p __attribute__((nonnull))) {
  return *p; // CHECK-NOT: [[# @LINE ]] warning
}

int nonnull_ptr_n_nn(int **p __attribute__((nonnull))) {
  return **p; // CHECK-NOT: [[# @LINE ]] warning
}

struct k1 {
  int **ptr;
};

int field_ptr_n_nn(struct k1 *p) {
  if (p == NULL)
    return 0;
  if (p->ptr == NULL)
    return 0;
  if (*(p->ptr) == NULL)
    return 0;
  return **(p->ptr); // CHECK-NOT: [[# @LINE ]] warning
}

int field1_ptr_n_nn(struct k1 *p) {
  int res = 0;
  if (p + 1 != NULL && (p + 1)->ptr != NULL && *((p + 1)->ptr) != NULL)
    res = **((p + 1)->ptr); // CHECK-NOT: [[# @LINE ]] warning
  if (p + 1 != NULL && (p + 1)->ptr != NULL && *((p + 1)->ptr) + 1 != NULL)
    res = *(*((p + 1)->ptr) + 1); // CHECK-NOT: [[# @LINE ]] warning
  if (p != NULL && p->ptr != NULL && *(p->ptr) + 1 != NULL)
    res =  *(*(p->ptr) + 1); // CHECK-NOT: [[# @LINE ]] warning
  return res;
}

struct k2 {
  int **ptr __attribute__((nonnull));
};

int nonnull_field_ptr_n_nn(struct k2 *p __attribute__((nonnull))) {
  return **(p->ptr); // CHECK-NOT: [[# @LINE ]] warning
}

int main() {
  return 0;
}
