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
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz6 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz6_a {
    int i_a() default 2;
    String t_a() default "";
}
@Zzz6(i = 333, t = "test1")
class GetAnnotation6 {
    public int i;
    public String t;
    @Zzz6_a(i_a = 666, t_a = "right1")
    public static class GetAnnotation6_a {
        public int j;
    }
}
public class ReflectionGetAnnotation6 {
    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotation6");
            Class[] zhu1 = zqp1.getDeclaredClasses();
            if (zhu1[0].getAnnotation(Zzz6_a.class).toString().indexOf("t_a=right1") != -1 && zhu1[0].
                    getAnnotation(Zzz6_a.class).toString().indexOf("i_a=666") != -1) {
                if (zhu1[0].getAnnotation(Zzz6.class) == null) {
                    System.out.println(0);
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}