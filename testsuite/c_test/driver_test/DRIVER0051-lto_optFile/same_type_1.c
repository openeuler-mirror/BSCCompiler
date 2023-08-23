/*
 * Copyright (c) Huawei Technologies Co., Ltd. 1987-2022. All rights reserved.
 * Description:
 * - @TestCaseID: same_type_1.c
 * - @TestCaseName: same_type_1.c
 * - @TestCaseType: Function Testing
 * - @RequirementID: AR.SR.IREQ4925321e.013.001
 * - @RequirementName: LTO特性支持
 * - @DesignDescription: Two type var have same name which in two files and -flto into a ELF
 * - #two type: struct with basic type member
 * - #member: same member
 * - #qualifier: blank
 * - @Condition: Lit test environment ready
 * - @Brief: Test boundary attribute -- function
 * - #step1: Call Function
 * - #step2: Check whether behavior of compilation or execution is expected
 * - @Expect: Lit exit 0.
 * - @Priority: Level 1
 */

struct Test {
    int x;
    int y;
};

int main()
{
    struct Test test1 = {1, 2};
    return 0;
}
// RUN: %CC %CFLAGS %s %S/flto/same_type_1b.c -flto -c
// RUN: %CC %CFLAGS same_type_1.o same_type_1b.o -flto -o %t
// RUN: %SIMULATOR %SIM_OPTS %t
