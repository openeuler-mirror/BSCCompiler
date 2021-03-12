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
abstract class GetDeclaredMethod3 {
    abstract void empty1();
    abstract public int empty2();
}
public class ReflectionGetDeclaredMethod3 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("GetDeclaredMethod3");
            Method method1 = clazz.getDeclaredMethod("empty1");
            Method method2 = clazz.getMethod("empty2");
            if (method1.toString().equals("abstract void GetDeclaredMethod3.empty1()")
                    && method2.toString().equals("public abstract int GetDeclaredMethod3.empty2()")) {
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