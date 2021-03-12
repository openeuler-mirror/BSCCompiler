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
    int 整型();
    byte 字节();
    char 字符();
    double 双精度浮点();
    boolean 布尔();
    long 长整();
    float 浮点();
    short 短整型();
    int[] あ();
    byte[] い();
    char[] う();
    double[] え();
    boolean[] お();
    long[] か();
    float[] き();
    short[] く();
    String 长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长长();
    String[] 神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马神马();
    Class 类();
    Class[] 类类();
    Thread.State 类类类();
    Thread.State[] 类类类类();
    AnnoB 类类类类类();
    AnnoB[] 类类类类类类();
}
