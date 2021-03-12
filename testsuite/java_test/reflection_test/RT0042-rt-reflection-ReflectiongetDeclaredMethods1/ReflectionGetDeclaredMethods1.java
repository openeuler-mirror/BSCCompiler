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
class GetDeclaredMethods1_a {
    public void empty1() {
    }
    public void empty2() {
    }
}
class GetDeclaredMethods1 extends GetDeclaredMethods1_a {
    public void void1() {
    }
    void void2() {
    }
    int getZero() {
        return 0;
    }
    private String getDd() {
        return "dd";
    }
    public void setNumber(int number) {
    }
    int setName(String name) {
        return 2;
    }
}
interface GetDeclaredMethods1_b {
    public default void test1() {
    }
    default void test2() {
    }
}
public class ReflectionGetDeclaredMethods1 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetDeclaredMethods1");
            Class clazz2 = Class.forName("GetDeclaredMethods1_b");
            Method[] methods1 = clazz1.getDeclaredMethods();
            Method[] methods2 = clazz2.getDeclaredMethods();
            if (methods1.length == 6 && methods2.length == 2) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        }
    }
}