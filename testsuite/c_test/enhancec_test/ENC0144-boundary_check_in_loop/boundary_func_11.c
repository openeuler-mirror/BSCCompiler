
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description:
 * - @TestCaseID: boundary_func_11.c
 * - @TestCaseName: boundary_func_11.c
 * - @TestCaseType: Function Testing
 * - @RequirementID: SR.IREQ9939ff5a.003
 * - @RequirementName: Boundary标注与越界静态分析和动态拦截
 * - @Design Description:
 * - #返回值数据类型: 无
 * - #返回值标注语法: 无
 * - #返回值标注位置: 无
 * - #返回值返回: 无
 * - #返回值运行时长度传入: 无
 * - #返回值场景是否可静态分析: 无
 * - #入参数据类型: double
 * - #入参标注语法: byte_count_index(var_len_index，var_index)
 * - #入参标注位置: RT* 【标注】func ( T* p )
 * - #入参传入: 数组地址运算
 * - #入参运行时长度传入: 合法值
 * - #入参场景是否可静态分析: 可静态分析
 * - #编译选项:  -boundary-check-static
 * - @Condition: Lit test environment ready
 * - @Brief: Test boundary attribute -- function
 * - #step1: Call Function
 * - #step2: Check whether behavior of compilation or execution is expected
 * - @Expect: Lit exit 0.
 * - @Priority: Level 1
 */

#include <stdio.h>

#define ARG_LEN 8
#define OFFSET 1
#define EXPECT (ARG_LEN - OFFSET)
#pragma SAFE ON

// pointer arg has arg_len elements
void __attribute__((byte_count_index(2, 1))) Func(short* arg, int arg_len);

void Func(short* arg, int arg_len)
{
    for (int i = 0; i < arg_len; i++) {
        arg[i] = 1; // CHECK: error: the pointer >= the upper bounds when accessing the memory and inlined to main
        *((int*)arg + i) = 1;
    }
}

int main(int argc, char* argv[])
{
    short arg[ARG_LEN] = {0};
    Func(arg + OFFSET, ARG_LEN - 1);
    short sum = 0;
    for(int i = OFFSET; i < ARG_LEN; i++) {
        sum += arg[i];
    }
    if (EXPECT != sum) {
        printf("Error: expect result is %d, but actual result is %d\n", EXPECT, sum);
        return 1;
    }
    return 0;
}

// RUN:not %CC %CFLAGS %DEBUG %BOUNDARY_STATIC_CHECK %s -o %t 2>&1 | FileCheck %s
// CHECK-DAG: 46 error
// CHECK-DAG: 45 warning
// CHECK-DAG: 46 warning
