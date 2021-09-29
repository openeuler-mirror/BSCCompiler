/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <stdlib.h>

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;

// u64
u64 __attribute__((noinline)) rot1(u64 a, u64 b) {
  return (a >> (b&63)) | (a << (64 - (b&63)));
}

u64 __attribute__((noinline)) rot2(u64 a, u64 b) {
  return (a << (64 - (b&63))) | (a >> (b&63));
}

// u32
u32 __attribute__((noinline)) rot3(u32 a, u32 b) {
  return (a >> (b&31)) | (a << (32 - (b&31)));
}

u32 __attribute__((noinline)) rot4(u32 a, u32 b) {
  return (a << (32 - (b&31))) | (a >> (b&31));
}

// u16
u16 __attribute__((noinline)) rot5(u16 a, u16 b) {
  return (a >> (b&15)) | (a << (16 - (b&15)));
}

u16 __attribute__((noinline)) rot6(u16 a, u16 b) {
  return (a << (16 - (b&15))) | (a >> (b&15));
}

int main() {
  u64 a = 9; // 1001
  u64 b = rot1(a, 1);
  u64 c = rot2(a, 2);
  u32 d = rot3(a, 3);
  u32 e = rot4(a, 4);
  u16 f = rot5(a, 5);
  u16 g = rot6(a, 6);
  printf("%lu, %lu, %u, %u, %u, %u\n", b, c, d, e, f, g);
  return 0;
}
