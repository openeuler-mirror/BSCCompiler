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

#include <stddef.h>

#include <stdio.h>
#include <float.h>

float MaxFloat(float a, float b)
{
    return (a - b) > FLT_MIN ? a : b;
}

double MaxDouble(double a, double b)
{
    return (a - b) > DBL_MIN ? a : b;
}

long double MaxLongDouble(long double a, long double b)
{
    return (a - b) > LDBL_MIN ? a : b;
}

int main()
{
    float floatA = -1.0;
    float floatB = 0.0;
    float floatMaxValue;
    double doubleA = 0.0;
    double doubleB = 1.0;
    double doubleMaxValue;
    long double lDoubleA = 0.0; // -0.0;
    long double lDoubleB = 0.0;
    long double lDoubleMaxValue;
    int errNum = 0;

    float (*floatMax)(float, float) __attribute__((nonnull)) = MaxFloat;
    double (*doubleTest)(double, double) __attribute__((nonnull)) = MaxDouble;
    double (*doubleMax)(double, double) __attribute__((nonnull)) = doubleTest;
    long double (*lDoubleTest1)(long double, long double) __attribute__((nonnull)) = MaxLongDouble;
    long double (*lDoubleTest2)(long double, long double) = lDoubleTest1;
    long double (*lDoubleMax)(long double, long double) __attribute__((nonnull)) = lDoubleTest2;

    floatMaxValue = (*floatMax)(floatA, floatB);
    doubleMaxValue = (*doubleMax)(doubleA, doubleB);
    lDoubleMaxValue = (*lDoubleMax)(lDoubleA, lDoubleB);

    errNum += ((floatMaxValue - MaxFloat(floatA, floatB)) < FLT_MIN ? 0 : 1);
    errNum += ((doubleMaxValue - MaxDouble(doubleA, doubleB)) < DBL_MIN ? 0 : 1);
    errNum += ((lDoubleMaxValue - MaxLongDouble(lDoubleA, lDoubleB)) < LDBL_MIN ? 0 : 1);

    return errNum;
}
