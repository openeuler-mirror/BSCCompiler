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


import java.lang.reflect.Method;
interface MethodIsDefault_A {
    default public void run() {
        String i;
    }
}
class MethodIsDefault implements MethodIsDefault_A {
    public void run() {
        String i = "abc";
    }
    void ss(int number) {
    }
}
public class RTMethodIsDefault {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("MethodIsDefault_A");
            Class clazz2 = Class.forName("MethodIsDefault");
            Method method1 = clazz1.getMethod("run");
            Method method2 = clazz2.getMethod("run");
            Method method3 = clazz2.getDeclaredMethod("ss", int.class);
            if (method1.isDefault()) {
                if (!method2.isDefault() && !method3.isDefault()) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e) {
            System.err.println(e);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
