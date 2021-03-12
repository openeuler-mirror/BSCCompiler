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
class MethodGetParameterCount2 {
    void test1() {
    }
    public void test2(String name, int number, char c, short s, float f, double d, long l, boolean b, byte bb) {
    }
}
public class RTMethodGetParameterCount {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodGetParameterCount2");
            Method method = clazz.getDeclaredMethod("test1");
            Method[] methods = clazz.getMethods();
            if (method.getParameterCount() == 0) {
                if (methods[0].getParameterCount() == 9) {
                    System.out.println(0);
                }
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