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
import java.lang.reflect.Method;
@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    public int i_a() default 2;
    String t_a() default "string default value";
    String s();
}
class MethodGetDefaultValue1 {
    @IF1(i = 333, t = "test1")
    public static void ii(String name) {
    }
    void tt(int number) {
    }
}
public class RTMethodGetDefaultValue1 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("IF1_a");
            Class clazz2 = Class.forName("IF1");
            Class clazz3 = Class.forName("MethodGetDefaultValue1");
            Method method1 = clazz1.getDeclaredMethod("i_a");
            Method method2 = clazz1.getMethod("t_a");
            Method method3 = clazz2.getDeclaredMethod("t");
            Method[] methods = clazz3.getMethods();
            Method method5 = clazz3.getDeclaredMethod("tt", int.class);
            Method method6 = clazz1.getDeclaredMethod("s");
            if ((int) method1.getDefaultValue() == 2
                    && method2.getDefaultValue().toString().equals("string default value")) {
                if (method3.getDefaultValue().equals("")) {
                    if (methods[0].getDefaultValue() == null && method5.getDefaultValue() == null) {
                        if (method6.getDefaultValue() == null) {
                            System.out.println(0);
                        }
                    }
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