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
#include <stdint.h>

// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"

void Test1(char a) {
  printf("u32->char = %d\n", a);
}

void Test2(int32_t a) {
  printf("i64->i32 = %d\n", a);
}

void Test3(int8_t a) {
  printf("i16->i8 = %d\n", a);
}

void Test4(uint16_t a) {
  printf("i64->u16 = %d\n", a);
}

void Test5(int8_t a) {
  printf("u16->i8 = %d\n", a);
}

void Test6(int32_t a) {
  printf("u64->i32 = %d\n", a);
}

void Test7(int16_t a) {
  printf("i8->i16 = %d\n", a);
}

void Test8(int64_t a) {
  printf("i16->i64 = %d\n", a);
}

void Test9(int32_t a) {
  printf("i16->i32 = %d\n", a);
}

void Test10(int16_t a) {
  printf("i32->i16 = %d\n", a);
}

int main() {
  unsigned long long int test_var_1;
  _Bool test_var_2;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 3]]
  // CHECK :   dassign %i_66_18 0 (zext u32 16 (ge u1 i32 (
  // CHECK :   sext i32 8 (sext i32 8 (dread u64 %test_var_1_61_26)),
  unsigned short i = (signed char)test_var_1 >= test_var_2 - 1;
  uint32_t a = 1000000;
  int64_t b = -300000;
  int16_t c = 300;
  uint16_t d = 64030;
  uint16_t e = 65535;
  uint64_t f = 99999999999;
  int8_t g = 10;
  int32_t h = 20000;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test1 (zext u32 8 (dread u32 %a_67_12))
  Test1(a);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test2 (cvt i32 i64 (dread i64 %b_68_11))
  Test2(b);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test3 (sext i32 8 (dread i32 %c_69_11))
  Test3(c);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test4 (zext u32 16 (dread i64 %b_68_11))
  Test4(b);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test5 (sext i32 8 (dread u32 %d_70_12))
  Test5(d);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test5 (sext i32 8 (dread u32 %e_71_12))
  Test5(e);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test6 (cvt i32 u64 (dread u64 %f_72_12))
  Test6(f);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test7 (sext i32 16 (dread i32 %g_73_10))
  Test7(g);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test8 (cvt i64 i32 (dread i32 %c_69_11))
  Test8(c);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test9 (sext i32 16 (dread i32 %c_69_11))
  Test9(c);
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2]]
  // CHECK :   call &Test10 (sext i32 16 (dread i32 %h_74_11))
  Test10(h);
  return 0;
}