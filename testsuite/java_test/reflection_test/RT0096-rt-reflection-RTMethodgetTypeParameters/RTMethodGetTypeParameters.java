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
import java.lang.reflect.TypeVariable;
class MethodGetTypeParameters2 {
    public <t, s, d, f, g> void ss(int number) {
    }
    void kk() {
    }
}
public class RTMethodGetTypeParameters {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodGetTypeParameters2");
            Method method1 = clazz.getMethod("ss", int.class);
            Method method2 = clazz.getDeclaredMethod("kk");
            TypeVariable[] typeVariables1 = method1.getTypeParameters();
            TypeVariable[] typeVariables2 = method2.getTypeParameters();
            if (typeVariables1.length == 5 && typeVariables2.length == 0) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}