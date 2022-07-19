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

// CHECK: [[# FILENUM:]] "{{.*}}/defaultSafe.c"

// CHECK: func &func0 public safed () void
void func0(void) {
  // CHECK: dassign %a{{.*}} 0 (constval i32 2)
  int a = 2;
  // CHECK-NOT: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: safe
  {
    // CHECK-NEXT: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NEXT: dassign %a{{.*}} 0 (constval i32 0)
    int a = 0;
    // CHECK-NOT: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NOT: endsafe
  }
}

// CHECK: func &main public safed () i32
__Safe__ int main() {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: unsafe
  __Unsafe__ {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %a{{.*}} 0 (constval i32 1)
  int a = 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: endunsafe
  }
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: safe
  __Safe__ {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %b{{.*}} 0 (constval i32 3)
  int b = 3;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: endsafe
  }
  // CHECK-NOT: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: safe
  {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK: dassign %c{{.*}} 0 (constval i32 4)
    int c = 4;
    // CHECK-NOT: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NOT: endsafe
  }
  return 0;
}

// CHECK: func &func5 public unsafed () void
__Unsafe__ void func5(void) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: unsafe
  __Unsafe__ {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %a{{.*}} 0 (constval i32 1)
  int a = 1;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: endunsafe
  }
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: safe
  __Safe__ {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %b{{.*}} 0 (constval i32 3)
  int b = 3;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NEXT: endsafe
  }
  // CHECK-NOT: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: safe
  {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK: dassign %c{{.*}} 0 (constval i32 4)
    int c = 4;
    // CHECK-NOT: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NOT: endsafe
  }
}

#pragma SAFE OFF
// CHECK: func &func1 public unsafed () void
void func1(void) {
}

// CHECK: func &func2 public safed () void
__Safe__ void func2(void){
}

#pragma SAFE ON

// CHECK: func &func3 public safed () void
void func3(void) {
}

// CHECK: func &func4 public unsafed () void
__Unsafe__ void func4(void) {
}
