/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
*/

import java.lang.annotation.*;
@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
public @interface AnnoA {
    int intA();
    byte byteA();
    char charA();
    double doubleA();
    boolean booleanA();
    long longA();
    float floatA();
    short shortA();
    int[] intAA();
    byte[] byteAA();
    char[] charAA();
    double[] doubleAA();
    boolean[] booleanAA();
    long[] longAA();
    float[] floatAA();
    short[] shortAA();
    String stringA();
    String[] stringAA();
    Class classA();
    Class[] classAA();
    Thread.State stateA();
    Thread.State[] stateAA();
    AnnoB annoBA();
    AnnoB[] annoBAA();
}
