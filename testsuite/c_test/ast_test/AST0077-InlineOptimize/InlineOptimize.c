/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
typedef void (*func)();

// CHECK:     type $TypeC_MNO[[# IDX:]] <struct {
// CHECK-NEXT:  @c i32 align(4),
// CHECK-NEXT:  @b <$TypeB_MNO[[# IDX:]]> align(4)}>
// CHECK-NEXT:type $TypeB_MNO[[# IDX:]] <struct {
// CHECK-NEXT:  @b i32 align(4),
// CHECK-NEXT:  @a <$TypeA_MNO[[# IDX:]]> align(4)}>
// CHECK-NEXT:type $TypeA_MNO[[# IDX:]] <struct {
// CHECK-NEXT:  @a i32 align(4)}>
struct TypeA {
  int a;
};

struct TypeB {
  int b;
  struct TypeA a;
};

struct TypeC {
  int c;
  struct TypeB b;
};

static int a = 1;

static void func1() {
  a++;
}

// CHECK: func &func2_MNO[[# IDX:]] public static used () void
static void func2() {
  a++;
}

// CHECK: func &func5 public used noinline () void
__attribute__((noinline))
void func5() {
}

// CHECK: func &func3_MNO[[# IDX:]] public static used () void
static void func3() {
  a++;
  func2();
  func f = func5;
  f();
}

void func4() {
  func3();
  struct TypeC c = {1};
}

__attribute__((noinline))
int main() {
  char *str = "123";
  func1();
  func2();
  return 0;
}
