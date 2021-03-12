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
@interface IF4 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF4_a {
    int c() default 0;
    String d() default "";
}
class ParameterGetDeclaredAnnotations2 {
    ParameterGetDeclaredAnnotations2() {
    }
    ParameterGetDeclaredAnnotations2(@IF4(i = 222, t = "Parameter") int number) {
    }
    public void test1(@IF4(i = 222, t = "Parameter") String name) {
    }
}
class ParameterGetDeclaredAnnotations2_a extends ParameterGetDeclaredAnnotations2 {
    ParameterGetDeclaredAnnotations2_a() {
    }
    ParameterGetDeclaredAnnotations2_a(@IF4_a(c = 666, d = "Happy new year") int number) {
    }
    public void test1(@IF4_a(c = 666, d = "Happy new year") String name) {
    }
}
class ParameterGetDeclaredAnnotations2_b extends ParameterGetDeclaredAnnotations2_a {
    ParameterGetDeclaredAnnotations2_b(int number) {
    }
    public void test1(String name) {
    }
}
public class RTParameterGetDeclaredAnnotations2 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ParameterGetDeclaredAnnotations2_b");
            Constructor cons = cls.getDeclaredConstructor(int.class);
            Method method = cls.getMethod("test1", String.class);
            Parameter[] p1 = cons.getParameters();
            Parameter[] p2 = method.getParameters();
            if (p1[0].getDeclaredAnnotations().length == 0 && p2[0].getDeclaredAnnotations().length == 0) {
                System.out.println(0);
                return;
            }
        } catch (ClassNotFoundException e) {
            System.out.println(1);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(2);
            return;
        }
        System.out.println(3);
        return;
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
