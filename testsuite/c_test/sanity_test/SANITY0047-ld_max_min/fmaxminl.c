
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description:
 * - @TestCaseID: fmaxl
 * - @TestCaseName: fmaxl
 * - @TestCaseType: Function Testing
 * - @RequirementID: SR.IREQ9939ff5a.001
 * - @RequirementName: C编译器能力
 * - @Design Description:
 * - #本例选取输入参数的全组合进行测试
 * - @Condition: Lit test environment ready
 * - @Brief: Test function fmaxl
 * - #step1: Call Function
 * - #step2: Check whether function returned value is expected
 * - @Expect: Code exit 0.
 * - @Priority: Level 1
 */

#include <stdio.h>
#include <math.h>
#include <float.h>

#define ARG0_0  ((-1) * LDBL_MAX)
#define ARG0_1  LDBL_MAX
#define ARG0_2  2.333333L

#define ARG1_0  ((-1) * LDBL_MAX)
#define ARG1_1  LDBL_MAX
#define ARG1_2  (-4.145213L)

#define ESPILON  0.000001

int TEST(long double x, long double y, long double expMax, long double expMin)
{
    long double resMax = fmaxl(x, y);
    long double resMin = fminl(x, y);
    long double absResMax  = (resMax - expMax) > 0 ? (resMax - expMax) : (expMax - resMax);
    long double absResMin = (resMin - expMin) > 0 ? (resMin - expMin) : (expMin - resMin);
    if (absResMax > ESPILON) {
        printf("Error: expect max result is %Lf, but actual result is %Lf.\n", expMax, resMax);
        return 0;
    }

    if (absResMin > ESPILON) {
        printf("Error: expect min result is %Lf, but actual result is %Lf.\n", expMin, resMin);
        return 0;
    }

    return 0;
}

int main()
{
    int ret = 0;
    ret += TEST(ARG0_0, ARG1_0, ARG1_0, ARG0_0);
    ret += TEST(ARG0_0, ARG1_1, ARG1_1, ARG0_0);
    ret += TEST(ARG0_0, ARG1_2, ARG1_2, ARG0_0);

    ret += TEST(ARG0_1, ARG1_0, ARG0_1, ARG1_0);
    ret += TEST(ARG0_1, ARG1_1, ARG0_1, ARG1_1);
    ret += TEST(ARG0_1, ARG1_2, ARG0_1, ARG1_2);

    ret += TEST(ARG0_2, ARG1_0, ARG0_2, ARG1_0);
    ret += TEST(ARG0_2, ARG1_1, ARG1_1, ARG0_2);
    ret += TEST(ARG0_2, ARG1_2, ARG0_2, ARG1_2);
    return ret;
}

// RUN: %CC %CFLAGS %s -lm -o %t.bin &> %t.build.log
// RUN: %SIMULATOR %SIM_OPTS %t.bin &> %t.run.log
// RUN: rm -rf %t.bin %t.build.log %t.run.log %S/Output/%basename_t.script