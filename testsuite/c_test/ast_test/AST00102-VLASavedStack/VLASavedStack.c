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

// CHECK: [[# FILENUM:]] "{{.*}}/VLASavedStack.c"
#include <stdio.h>

int main() {
	return 0;
}

int foo (int n) { // scope 0
  printf("0");
  { // scope 1
		printf("1");
    int vla[n];
		if (n > 0) {  // scope 2
			int vla1[n];
			printf("2");
			if (n > 5) { // scope 3
				// CHECK: LOC [[# FILENUM]] [[# @LINE + 3 ]]
        // CHECK-NEXT: intrinsiccall C_stack_restore
				// CHECK-NEXT: intrinsiccall C_stack_restore
        goto i_am_here_2;
			}
		}
    printf("failed;");
		if (n < 0) { // scope 4
	  	i_am_here_1:
		  printf("4");
		}
	}
	// CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
	// CHECK-NOT: intrinsiccall C_stack_restore
  goto i_am_here_2;
  printf("failed;");
  i_am_here_2:
  printf("5");
  return 0;
}

