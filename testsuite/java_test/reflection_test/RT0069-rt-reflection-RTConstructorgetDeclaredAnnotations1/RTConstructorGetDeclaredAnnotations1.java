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
@Target(ElementType.CONSTRUCTOR)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.CONSTRUCTOR)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF1_a {
    int i_a() default 2;
    String t_a() default "";
}
class ConstructorGetDeclaredAnnotations1 {
    public ConstructorGetDeclaredAnnotations1() {
    }
    @IF1(i = 333, t = "test1")
    public ConstructorGetDeclaredAnnotations1(String name) {
    }
    @IF1(i = 333, t = "test1")
    ConstructorGetDeclaredAnnotations1(int number) {
    }
}
class ConstructorGetDeclaredAnnotations1_a extends ConstructorGetDeclaredAnnotations1 {
    @IF1_a(i_a = 666, t_a = "right1")
    @IF1(i = 333, t = "test1")
    public ConstructorGetDeclaredAnnotations1_a(String name) {
    }
    @IF1_a(i_a = 666, t_a = "right1")
    ConstructorGetDeclaredAnnotations1_a(int number) {
    }
}
public class RTConstructorGetDeclaredAnnotations1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorGetDeclaredAnnotations1_a");
            Constructor instance1 = cls.getConstructor(String.class);
            if (instance1.getDeclaredAnnotations().length == 2) {
                Constructor instance2 = cls.getDeclaredConstructor(int.class);
                if (instance2.getDeclaredAnnotations().length == 1) {
                    Annotation[] j = instance2.getDeclaredAnnotations();
                    if (j[0].toString().indexOf("i_a=666") != -1 && j[0].toString().indexOf("t_a=right1") != -1) {
                        System.out.println(0);
                        return;
                    }
                }
                System.out.println(1);
                return;
            }
            System.out.println(2);
            return;
        } catch (ClassNotFoundException e) {
            System.out.println(3);
            return;
        } catch (NoSuchMethodException e1) {
            System.out.println(4);
            return;
        } catch (NullPointerException e2) {
            System.out.println(5);
            return;
        }
    }
}