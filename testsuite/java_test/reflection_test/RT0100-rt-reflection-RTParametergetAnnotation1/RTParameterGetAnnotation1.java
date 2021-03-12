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
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.Parameter;
@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    int c() default 0;
    String d() default "";
}
class ParameterGetAnnotation1 {
    ParameterGetAnnotation1(@IF1(i = 222, t = "Parameter") int number) {
    }
    public ParameterGetAnnotation1(String name) {
    }
    public void test1(@IF1_a(c = 666, d = "Happy new year") int age) {
    }
}
public class RTParameterGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetAnnotation1");
            Constructor cons1 = cls.getDeclaredConstructor(int.class);
            Constructor cons2 = cls.getConstructor(String.class);
            Method method = cls.getMethod("test1", int.class);
            Parameter[] p1 = cons1.getParameters();
            Parameter[] p2 = cons2.getParameters();
            Parameter[] p3 = method.getParameters();
            if (p1[0].getAnnotation(IF1.class).i() == 222 && p1[0].getAnnotation(IF1.class).t().equals("Parameter")
                    && p3[0].getAnnotation(IF1_a.class).c() == 666
                    && p3[0].getAnnotation(IF1_a.class).d().equals("Happy new year")) {
                if (p2[0].getAnnotation(IF1.class) == null) {
                    System.out.println(0);
                    return;
                }
                System.out.println(2);
                return;
            }
        } catch (ClassNotFoundException e) {
            System.out.println(3);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(4);
            return;
        }
        System.out.println(5);
    }
}