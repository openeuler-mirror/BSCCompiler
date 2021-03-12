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
@interface interface2 {
    int num() default 0;
    String str() default "";
}
@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface interface2_a {
    int i_a() default 2;
    String t_a() default "";
}
class FieldGetDeclaredAnnotations2 {
    @interface2(num = 333, str = "test1")
    public int num;
    @interface2(num = 333, str = "test1")
    public String str;
}
class FieldGetDeclaredAnnotations2_a extends FieldGetDeclaredAnnotations2 {
    @interface2_a(i_a = 666, t_a = "right1")
    public int num;
    public String str;
}
public class RTFieldGetDeclaredAnnotations2 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGetDeclaredAnnotations2_a");
            Field instance1 = cls.getField("str");
            Field instance2 = cls.getDeclaredField("str");
            if (instance1.getDeclaredAnnotations().length == 0 && instance2.getDeclaredAnnotations().length == 0) {
                System.out.println(0);
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