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
@interface IF8 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF8_a {
    int c() default 0;
    String d() default "";
}
class ParameterGetAnnotationsByType2 {
    ParameterGetAnnotationsByType2(@IF8(i = 222, t = "Parameter") int number) {
    }
    public void test1(@IF8_a(c = 666, d = "Happy new year") int age) {
    }
}
public class RTParameterGetAnnotationsByType2 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetAnnotationsByType2");
            Constructor cons = cls.getDeclaredConstructor(int.class);
            Method method = cls.getMethod("test1", int.class);
            Parameter[] p1 = cons.getParameters();
            Parameter[] p2 = method.getParameters();
            Annotation[] q1 = p1[0].getAnnotationsByType(IF8_a.class);
            Annotation[] q2 = p2[0].getAnnotationsByType(IF8.class);
            q1[0].toString();
            q2[0].toString();
        } catch (ClassNotFoundException e) {
            System.out.println(1);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(2);
            return;
        } catch (ArrayIndexOutOfBoundsException e2) {
            try {
                Class cls1 = Class.forName("ParameterGetAnnotationsByType2");
                Constructor cons1 = cls1.getDeclaredConstructor(int.class);
                Parameter[] p3 = cons1.getParameters();
                Annotation[] q3 = p3[0].getAnnotationsByType(null);
                System.out.println(3);
                return;
            } catch (ClassNotFoundException e3) {
                System.out.println(4);
                return;
            } catch (NoSuchMethodException e4) {
                System.out.println(5);
                return;
            } catch (NullPointerException e5) {
                System.out.println(0);
                return;
            }
        }
        System.out.println(6);
        return;
    }
}