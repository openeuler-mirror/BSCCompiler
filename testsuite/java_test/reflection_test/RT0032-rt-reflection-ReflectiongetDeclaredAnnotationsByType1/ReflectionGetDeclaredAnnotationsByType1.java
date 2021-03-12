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
@interface Dddd1 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Dddd1_a {
    int i_a() default 2;
    String t_a() default "";
}
@Dddd1(i = 333, t = "test1")
class GetDeclaredAnnotationsByType1 {
    public int i;
    public String t;
}
@Dddd1_a(i_a = 666, t_a = "right1")
class GetDeclaredAnnotationsByType1_a extends GetDeclaredAnnotationsByType1 {
    public int i_a;
    public String t_a;
}
class GetDeclaredAnnotationsByType1_b extends GetDeclaredAnnotationsByType1_a {
}
public class ReflectionGetDeclaredAnnotationsByType1 {
    public static void main(String[] args) {
        int result = 0; /* STATUS_Success*/

        try {
            Class zqp1 = Class.forName("GetDeclaredAnnotationsByType1_b");
            Class zqp2 = Class.forName("GetDeclaredAnnotationsByType1_a");
            if (zqp1.getDeclaredAnnotationsByType(Dddd1.class).length == 0) {
                Annotation[] j = zqp2.getDeclaredAnnotationsByType(Dddd1_a.class);
                if (j[0].toString().indexOf("i_a=666") != -1 && j[0].toString().indexOf("t_a=right1") != -1) {
                    result = 0;
                }
            }
        } catch (ClassNotFoundException e1) {
            System.err.println(e1);
            result = -1;
        } catch (NullPointerException e2) {
            System.err.println(e2);
            result = -1;
        }
        System.out.println(result);
    }
}