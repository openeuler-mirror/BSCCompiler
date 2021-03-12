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


import java.lang.reflect.Field;
class FieldGetDeclaringClass {
    public int num;
    char aChar;
}
public class RTFieldGetDeclaringClass {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGetDeclaringClass");
            Field instance1 = cls.getField("num");
            Field instance2 = cls.getDeclaredField("aChar");
            Class<?> j = instance1.getDeclaringClass();
            Class<?> k = instance2.getDeclaringClass();
            if (j.getName().equals("FieldGetDeclaringClass") && k.getName().equals("FieldGetDeclaringClass")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchFieldException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
