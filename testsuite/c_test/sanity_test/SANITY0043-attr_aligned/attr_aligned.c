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

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#define STR(s) #s

#define SHOW_STRUCT(t)                                                         \
  do {                                                                         \
    printf(STR(t) " is size %zd, align %zd\n", sizeof(struct t),               \
           _Alignof(struct t));                                                \
    printf("  a is at offset %zd\n", offsetof(struct t, a));                   \
    printf("  b is at offset %zd\n", offsetof(struct t, b));                   \
  } while (0)


struct __attribute__((aligned(16))) my_struct
{
  int a;
  char b;
};

struct my_struct_reduced
{
  char a;
  int b __attribute__((aligned (2)));
};

struct my_struct_packed
{
  char a;
  int b __attribute__((packed)) __attribute__((aligned (2)));
};

int main() {
  SHOW_STRUCT(my_struct);
  SHOW_STRUCT(my_struct_reduced);
  SHOW_STRUCT(my_struct_packed);
  return 0;
}
