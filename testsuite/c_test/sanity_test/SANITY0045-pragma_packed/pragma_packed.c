/*
 * Copyright (C) [2022] Futurewei Technologies, Inc. All rights reverved.
 *
 * Licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include <stdio.h>
#include <stddef.h>

struct A {
  int a;
  char b;
  long c;
};

#pragma pack(push, 1)

struct B {
  int a;
  char b;
  long c;
};

int main() {
  printf("A size %zd, A align %zd\n", sizeof(struct A), _Alignof(struct A));
  printf("a is at offset %zd\n", offsetof(struct A, a));
  printf("b is at offset %zd\n", offsetof(struct A, b));
  printf("c is at offset %zd\n", offsetof(struct A, c));

  printf("B size %zd, B align %zd\n", sizeof(struct B), _Alignof(struct B));
  printf("a is at offset %zd\n", offsetof(struct B, a));
  printf("b is at offset %zd\n", offsetof(struct B, b));
  printf("c is at offset %zd\n", offsetof(struct B, c));

  return 0;
}

