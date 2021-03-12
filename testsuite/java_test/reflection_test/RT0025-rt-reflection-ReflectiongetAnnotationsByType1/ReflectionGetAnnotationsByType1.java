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
@interface Zzzz1 {
    int i() default 0;
    String t() default "";
}
@interface Zzzz1_a {
}
@interface Zzzz1_b {
}
class GetAnnotationsByType1_a {
    @Zzzz1(i = 333, t = "GetAnnotationsByType")
    public int i_a;
    @Zzzz1(i = 333, t = "GetAnnotationsByType")
    public int i_aa;
    @Zzzz1(i = 333, t = "GetAnnotationsByType")
    public int i_aaa;
    public String t_a;
    @Zzzz1_a
    public String t_aa;
    @Zzzz1_b
    public String t_aaa;
}
public class ReflectionGetAnnotationsByType1 {
    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotationsByType1_a");
            Field zhu1 = zqp1.getField("i_a");
            Field zhu2 = zqp1.getField("t_a");
            if (zqp1.getAnnotationsByType(Zzzz1.class).length == 0) {
                if (zhu2.getAnnotationsByType(Zzzz1_a.class).length == 0) {
                    Annotation[] k = zhu1.getAnnotationsByType(Zzzz1.class);
                    if (k[0].toString().indexOf("t=GetAnnotationsByType") != -1 && k[0].toString().indexOf("i=333")
                            != -1) {
                        System.out.println(0);
                    }
                }
            }
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchFieldException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            System.err.println(e2);
            System.out.println(2);
        }
    }
}