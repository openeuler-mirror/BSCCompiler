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
class GetMethod2_a {
    public int getName(String name) {
        return 10;
    }
    public String getString() {
        return "dda";
    }
}
class GetMethod2 extends GetMethod2_a {
    public void getVoid() {
    }
    void empty() {
    }
    int getZero() {
        return 0;
    }
    private String getStr() {
        return "dd";
    }
    public void setNum(int number) {
    }
    int getSecondNum(String name) {
        return 2;
    }
}
public class ReflectionGetMethod2 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetMethod2");
            Method method1 = clazz1.getMethod("empty");
            System.out.println(2);
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            // NoSuchMethodException is thrown when method not exist.
            try {
                Class clazz2 = Class.forName("GetMethod2");
                Method method2 = clazz2.getMethod(null);
                System.out.println(2);
            } catch (ClassNotFoundException e4) {
                System.err.println(e4);
                System.out.println(2);
            } catch (NoSuchMethodException e5) {
                System.err.println(e5);
                System.out.println(2);
            } catch (NullPointerException e6) {
                // Expected result: NullPointerException is thrown when method is null.
                System.out.println(0);
            }
        } catch (NullPointerException e3) {
            System.err.println(e3);
            System.out.println(2);
        }
    }
}