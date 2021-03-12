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
@interface IFw1 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IFw1_a {
    int i_a() default 2;
    String t_a() default "";
}
class MethodGetDeclaredAnnotations1 {
    @IFw1(i = 333, t = "test1")
    public static void ii(String name) {
    }
    @IFw1(i = 333, t = "test1")
    void tt(int number) {
    }
}
class MethodGetDeclaredAnnotations1_a extends MethodGetDeclaredAnnotations1 {
    @IFw1_a(i_a = 666, t_a = "right1")
    public static void ii(String name) {
    }
    @IFw1_a(i_a = 666, t_a = "right1")
    void tt(int number) {
    }
}
public class RTMethodGetDeclaredAnnotations1 {
    public static void main(String[] args) {
        try {
            Class clazz = Class.forName("MethodGetDeclaredAnnotations1_a");
            Method method1 = clazz.getMethod("ii", String.class);
            if (method1.getDeclaredAnnotations().length == 1) {
                Annotation[] annotations1 = method1.getDeclaredAnnotations();
                if (annotations1[0].toString().indexOf("i_a=666") != -1
                        && annotations1[0].toString().indexOf("t_a=right1") != -1) {
                    Method method2 = clazz.getDeclaredMethod("tt", int.class);
                    if (method2.getDeclaredAnnotations().length == 1) {
                        Annotation[] annotations2 = method2.getDeclaredAnnotations();
                        if (annotations2[0].toString().indexOf("i_a=666") != -1
                                && annotations2[0].toString().indexOf("t_a=right1") != -1) {
                            System.out.println(0);
                        }
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}