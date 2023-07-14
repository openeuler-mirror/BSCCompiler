/*
 * Copyright (c) [2023-2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <stdlib.h>
struct A {
  int len;
  int fa[] __attribute__((count("len")));
};

void check_flex_array_assign() {
  struct A *a = (struct A*)malloc(sizeof(struct A) + 10);
  int *t;
  //CHECK: dassign %_boundary.t_{{.*}}.lower 0 (iaddrof ptr <* <$A>> 2 (dread ptr %a_{{.*}}))
  //CHECK: dassign %_boundary.t_{{.*}}.upper 0 (add ptr (
  //CHECK:    iaddrof ptr <* <$A>> 2 (dread ptr %a_{{.*}}),
  //CHECK:    mul i64 (
  //CHECK:      cvt i64 i32 (iread i32 <* <$A>> 1 (dread ptr %a_{{.*}})),
  //CHECK:      constval i64 4)))
  t = a->fa;
}

int main() {
  return 0;
}
