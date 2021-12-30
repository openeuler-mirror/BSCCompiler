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

// CHECK: [[# FILENUM:]] "{{.*}}/safe_region.c"

// CHECK: func &func0 public () void
void func0(void) {
}

#pragma SAFE OFF

// CHECK: func &func1 public unsafed () void
void func1(void) {
}

// CHECK: func &func2 public safed () void
__Safe__ void func2(void);

#pragma SAFE ON

// CHECK: func &func3 public unsafed () void
__Unsafe__ void func3(void) {
}

// CHECK: func &func4 public safed () void
void func4() {
}

// CHECK: func &main public safed () i32
int main(void) {
  return 0;
}

__Safe__ void func2(void) {
  // CHECK: dassign %a_48_3 0 (constval i32 0)
  int a = 0;
  // CHECK-NEXT: safe
  __Safe__ {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NEXT: dassign %a_48_3 0 (add i32 (dread i32 %a_48_3, constval i32 1))
    a += 1;
    // CHECK-NEXT: unsafe
    __Unsafe__ {
      // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
      // CHECK-NEXT: dassign %a_48_3 0 (add i32 (dread i32 %a_48_3, constval i32 2))
      a += 2;
      // CHECK-NEXT: safe
      __Safe__ {
        // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
        // CHECK-NEXT: dassign %a_48_3 0 (add i32 (dread i32 %a_48_3, constval i32 3))
        a += 3;
      // CHECK-NEXT: endsafe
      }
      // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
      // CHECK-NEXT: dassign %a_48_3 0 (add i32 (dread i32 %a_48_3, constval i32 4))
      a += 4;
    // CHECK-NEXT: endunsafe
    }
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
    // CHECK-NEXT: dassign %a_48_3 0 (add i32 (dread i32 %a_48_3, constval i32 5))
    a += 5;
  // CHECK-NEXT: endsafe
  }
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]{{$}}
  // CHECK-NEXT: dassign %a_48_3 0 (add i32 (dread i32 %a_48_3, constval i32 6))
  a += 6;
}
