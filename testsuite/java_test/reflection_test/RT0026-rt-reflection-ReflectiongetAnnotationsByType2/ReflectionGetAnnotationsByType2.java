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
import java.lang.reflect.Field;
@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzzz2 {
    int i() default 0;
    String t() default "";
}
class GetAnnotationsByType2_a {
    @Zzzz2(i = 333, t = "GetAnnotationsByType")
    public int i_a;
    public String t_a;
}
class GetAnnotationsByType2_b {
    public int i_b;
    public String t_b;
}
public class ReflectionGetAnnotationsByType2 {
    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotationsByType2_a");
            Field zhu1 = zqp1.getField("t_a");
            Annotation[] j = zhu1.getAnnotationsByType(null);
            System.out.println(2);
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchFieldException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            try {
                Class zqp2 = Class.forName("GetAnnotationsByType2_b");
                Field zhu2 = zqp2.getField("i_b");
                Annotation[] k = zhu2.getAnnotationsByType(null);
                System.out.println(2);
            } catch (ClassNotFoundException e3) {
                System.err.println(e3);
                System.out.println(2);
            } catch (NoSuchFieldException e4) {
                System.err.println(e4);
                System.out.println(2);
            } catch (NullPointerException e5) {
                System.out.println(0);
            }
        }
    }
}