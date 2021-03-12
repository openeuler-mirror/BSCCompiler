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
@interface IF7 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF7_a {
    int c() default 0;
    String d() default "";
}
class ParameterGetAnnotationsByType1 {
    ParameterGetAnnotationsByType1(@IF7(i = 222, t = "Parameter") int number) {
    }
    public ParameterGetAnnotationsByType1(String name) {
    }
    public void test1(@IF7_a(c = 666, d = "Happy new year") int age) {
    }
}
public class RTParameterGetAnnotationsByType1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetAnnotationsByType1");
            Constructor cons1 = cls.getDeclaredConstructor(int.class);
            Constructor cons2 = cls.getConstructor(String.class);
            Method method = cls.getMethod("test1", int.class);
            Parameter[] p1 = cons1.getParameters();
            Parameter[] p2 = cons2.getParameters();
            Parameter[] p3 = method.getParameters();
            Annotation[] q1 = p1[0].getAnnotationsByType(IF7.class);
            Annotation[] q3 = p3[0].getAnnotationsByType(IF7_a.class);
            if (p2[0].getAnnotationsByType(IF7.class).length == 0) {
                if (q1[0].toString().indexOf("i=222") != -1 && q1[0].toString().indexOf("t=Parameter") != -1
                        && q3[0].toString().indexOf("c=666") != -1
                        && q3[0].toString().indexOf("d=Happy new year") != -1) {
                    System.out.println(0);
                    return;
                }
                System.out.println(1);
                return;
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(3);
            return;
        }
        System.out.println(4);
        return;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
