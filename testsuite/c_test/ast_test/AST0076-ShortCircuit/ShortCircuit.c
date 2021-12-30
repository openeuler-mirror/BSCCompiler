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

int func1() {
  return 1;
}

int main() {
  int a, b, c;
  // CHECK:       if (cior i32 (
  // CHECK-NEXT:    ne u1 i32 (dread i32 %a_21_3, constval i32 0),
  // CHECK-NEXT:    ne u1 i32 (dread i32 %b_21_3, constval i32 0))) {
  // CHECK-NEXT:  }
  if (a || b) {
  }
  // CHECK:       dassign %shortCircuit_[[# IDX:]] 0 (cior i32 (
  // CHECK-NEXT:        ne u1 i32 (dread i32 %a_21_3, constval i32 0),
  // CHECK-NEXT:        ne u1 i32 (dread i32 %b_21_3, constval i32 0)))
  // CHECK-NEXT:    brtrue @shortCircuit_label_[[# IDX:]] (dread i32 %shortCircuit_[[# IDX:]])
  // CHECK-NEXT:    callassigned &func1 () { dassign %retVar_[[# IDX:]] 0 }
  // CHECK-NEXT:    dassign %shortCircuit_[[# IDX:]] 0 (ne u1 i32 (dread i32 %retVar_[[# IDX:]], constval i32 0))
  // CHECK-NEXT:  @shortCircuit_label_[[# IDX:]]   if (ne u1 i32 (dread i32 %shortCircuit_[[# IDX:]], constval i32 0)) {
  // CHECK-NEXT:  }
  if (a || b || func1()) {
  }
  // CHECK:       callassigned &func1 () { dassign %retVar_[[# IDX:]] 0 }
  // CHECK-NEXT:  if (cior i32 (
  // CHECK-NEXT:    cior i32 (
  // CHECK-NEXT:      ne u1 i32 (dread i32 %retVar_[[# IDX:]], constval i32 0),
  // CHECK-NEXT:      ne u1 i32 (dread i32 %a_21_3, constval i32 0)),
  // CHECK-NEXT:    ne u1 i32 (dread i32 %b_21_3, constval i32 0))) {
  // CHECK-NEXT:  }
  if (func1() || a || b) {
  }
  // CHECK:        dassign %shortCircuit_[[# IDX:]] 0 (ne u1 i32 (dread i32 %c_21_3, constval i32 0))
  // CHECK-NEXT:   brtrue @shortCircuit_label_[[# IDX:]] (dread i32 %shortCircuit_[[# IDX:]])
  // CHECK-NEXT:   callassigned &func1 () { dassign %retVar_[[# IDX:]] 0 }
  // CHECK-NEXT:   dassign %shortCircuit_[[# IDX:]] 0 (ne u1 i32 (dread i32 %retVar_[[# IDX:]], constval i32 0))
  // CHECK-NEXT: @shortCircuit_label_[[# IDX:]]   if (cior i32 (
  // CHECK-NEXT:    cior i32 (
  // CHECK-NEXT:      ne u1 i32 (dread i32 %shortCircuit_[[# IDX:]], constval i32 0),
  // CHECK-NEXT:      ne u1 i32 (dread i32 %a_21_3, constval i32 0)),
  // CHECK-NEXT:    ne u1 i32 (dread i32 %b_21_3, constval i32 0))) {
  // CHECK-NEXT:  }
  if (c || func1() || a || b) {
  }
  return 0;
}
