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
@interface IF2 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.CONSTRUCTOR)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface IF2_a {
    int c() default 0;
    String d() default "";
}
class ConstructorGetAnnotation2 {
    @IF2(i = 333, t = "ConstructorGetAnnotation")
    public ConstructorGetAnnotation2() {
    }
    @IF2_a(c = 666, d = "Constructor")
    ConstructorGetAnnotation2(int number) {
    }
}
class ConstructorGetAnnotation2_a {
    ConstructorGetAnnotation2_a(String name, int number) {
    }
}
public class RTConstructorGetAnnotation2 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("ConstructorGetAnnotation2");
            Constructor instance1 = cls.getDeclaredConstructor(int.class);
            Constructor instance2 = cls.getConstructor();
            if (instance1.getAnnotation(IF2.class) == null && instance2.getAnnotation(IF2_a.class) == null) {
                instance1.getAnnotation(IF2.class).toString();
            }
            System.out.println(1);
            return;
        } catch (ClassNotFoundException e1) {
            System.out.println(1);
            return;
        } catch (NoSuchMethodException e2) {
            System.out.println(1);
            return;
        } catch (NullPointerException e3) {
            try {
                Class cls = Class.forName("ConstructorGetAnnotation2_a");
                Constructor instance3 = cls.getDeclaredConstructor(String.class, int.class);
                if (instance3.getAnnotation(IF2.class) == null && instance3.getAnnotation(IF2_a.class) == null) {
                    instance3.getAnnotation(IF2.class).toString();
                }
                System.out.println(1);
                return;
            } catch (ClassNotFoundException e4) {
                System.out.println(2);
                return;
            } catch (NoSuchMethodException e5) {
                System.out.println(3);
                return;
            } catch (NullPointerException e6) {
                System.out.println(0);
                return;
            }
        }
    }
}