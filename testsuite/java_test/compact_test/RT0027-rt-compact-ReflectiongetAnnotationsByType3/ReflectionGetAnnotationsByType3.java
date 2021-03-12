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
@interface Zzzz3 {
    int i() default 0;
    String t() default "";
}
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzzz3_a {
    int i_a() default 2;
    String t_a() default "";
}
@Zzzz3(i = 333, t = "test1")
class GetAnnotationsByType3 {
    public int i;
    public String t;
}
@Zzzz3_a(i_a = 666, t_a = "right1")
class GetAnnotationsByType3_a extends GetAnnotationsByType3 {
    public int i_a;
    public String t_a;
}
class GetAnnotationsByType3_b extends GetAnnotationsByType3_a {
}
public class ReflectionGetAnnotationsByType3 {
    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotationsByType3_b");
            Annotation[] j = zqp1.getAnnotationsByType(Zzzz3.class);
            if (j[0].toString().indexOf("t=test1") != -1 && j[0].toString().indexOf("i=333") != -1) {
                System.out.println(0);
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
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
