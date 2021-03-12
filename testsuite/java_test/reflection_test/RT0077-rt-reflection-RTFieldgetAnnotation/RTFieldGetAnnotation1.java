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
@interface interface4 {
    int num() default 0;
    String str() default "";
}
@interface interface4_a {
    int number() default 0;
    String string() default "";
}
class FieldGetAnnotation1 {
    @interface4(num = 333, str = "GetAnnotation")
    public int num1;
    @interface4_a(number = 555, string = "test")
    public String str1;
}
public class RTFieldGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class cls1 = Class.forName("FieldGetAnnotation1");
            Field instance1 = cls1.getField("num1");
            Field instance2 = cls1.getField("str1");
            if (instance1.getAnnotation(interface4_a.class) == null && instance2.getAnnotation(interface4.class) == null)
            {
                instance1.getAnnotation(interface4_a.class).number();
            }
            System.out.println(2);
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchFieldException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            System.out.println(0);
        }
    }
}