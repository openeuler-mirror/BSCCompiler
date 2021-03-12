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
    int i_a() default 0;
    String t_b() default "";
}
@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    int c() default 0;
    String d() default "";
}
class MethodGetAnnotation1 {
    @IF1(i_a = 333, t_b = "MethodGetAnnotation")
    public void test1() {
    }
    public void test2(String name) {
    }
    @IF1_a(c = 666, d = "Method")
    void test3(int number) {
    }
    void test4(String name, int number) {
    }
}
public class RTMethodGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodGetAnnotation1");
            Method method1 = clazz.getDeclaredMethod("test3", int.class);
            Method method2 = clazz.getMethod("test1");
            Method method3 = clazz.getMethod("test2", String.class);
            if (method1.getAnnotation(IF1_a.class).c() == 666
                    && method1.getAnnotation(IF1_a.class).d().equals("Method")
                    && method2.getAnnotation(IF1.class).i_a() == 333
                    && method2.getAnnotation(IF1.class).t_b().equals("MethodGetAnnotation")) {
                if (method3.getAnnotation(IF1.class) == null) {
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