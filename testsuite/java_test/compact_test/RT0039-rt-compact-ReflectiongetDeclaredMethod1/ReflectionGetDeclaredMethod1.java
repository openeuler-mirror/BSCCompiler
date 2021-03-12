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
class GetDeclaredMethod1 {
    public void empty1() {
    }
    void empty2() {
    }
    int getZero() {
        return 0;
    }
    private String getDd() {
        return "dd";
    }
    public void empty1(int number) {
    }
    int getZero(String name) {
        return 2;
    }
}
public class ReflectionGetDeclaredMethod1 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("GetDeclaredMethod1");
            Method method1 = clazz.getDeclaredMethod("empty1", int.class);
            Method method2 = clazz.getDeclaredMethod("getDd");
            Method method3 = clazz.getDeclaredMethod("empty1");
            Method method4 = clazz.getDeclaredMethod("empty2");
            Method method5 = clazz.getDeclaredMethod("getZero");
            Method method6 = clazz.getDeclaredMethod("getZero", String.class);
            if (method1.toString().equals("public void GetDeclaredMethod1.empty1(int)")
                    && method2.toString().equals("private java.lang.String GetDeclaredMethod1.getDd()")
                    && method3.toString().equals("public void GetDeclaredMethod1.empty1()")
                    && method4.toString().equals("void GetDeclaredMethod1.empty2()")
                    && method5.toString().equals("int GetDeclaredMethod1.getZero()")
                    && method6.toString().equals("int GetDeclaredMethod1.getZero(java.lang.String)")) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NoSuchMethodException e2) {
            System.err.println(e2);
            System.out.println(2);
        } catch (NullPointerException e3) {
            System.err.println(e3);
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
