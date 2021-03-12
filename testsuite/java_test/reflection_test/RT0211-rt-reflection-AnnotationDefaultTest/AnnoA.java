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
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
public @interface AnnoA {
    int intA() default Integer.MAX_VALUE;
    byte byteA() default Byte.MAX_VALUE;
    char charA() default Character.MAX_VALUE;
    double doubleA() default Double.NaN;
    boolean booleanA() default true;
    long longA() default Long.MAX_VALUE;
    float floatA() default Float.NaN;
    short shortA() default Short.MAX_VALUE;
    int[] intAA() default {1,2};
    byte[] byteAA() default {0};
    char[] charAA() default {' '};
    double[] doubleAA() default {Double.NaN, Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY};
    boolean[] booleanAA() default {true};
    long[] longAA() default {Long.MAX_VALUE};
    float[] floatAA() default {Float.NaN, Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY};
    short[] shortAA() default {0};
    String stringA() default "";
    String[] stringAA() default "";
    Class classA() default Thread.class;
    Class[] classAA() default Thread.class;
    Thread.State stateA() default Thread.State.BLOCKED;
    Thread.State[] stateAA() default Thread.State.NEW;
    AnnoB annoBA() default  @AnnoB;
    AnnoB[] annoBAA() default {@AnnoB, @AnnoB};
}
