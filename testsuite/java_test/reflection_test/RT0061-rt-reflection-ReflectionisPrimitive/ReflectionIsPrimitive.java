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


public class ReflectionIsPrimitive {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        Class zqp1 = ReflectionIsPrimitive.class;
        Class zqp2 = int.class;
        Class zqp3 = boolean.class;
        Class zqp4 = byte.class;
        Class zqp5 = char.class;
        Class zqp6 = short.class;
        Class zqp7 = long.class;
        Class zqp8 = float.class;
        Class zqp9 = double.class;
        Class zqp10 = void.class;
        if (!zqp1.isPrimitive() && zqp2.isPrimitive() && zqp3.isPrimitive() && zqp4.isPrimitive() && zqp5.isPrimitive()
                && zqp6.isPrimitive() && zqp7.isPrimitive() && zqp8.isPrimitive() && zqp9.isPrimitive() &&
                zqp10.isPrimitive()) {
            result = 0;
        }
        System.out.println(result);
    }
}