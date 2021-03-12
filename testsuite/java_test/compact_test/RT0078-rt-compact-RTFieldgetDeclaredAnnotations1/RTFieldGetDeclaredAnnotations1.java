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
@interface interface1 {
    int num() default 0;
    String str() default "";
}
@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface interface1_a {
    int num_a() default 2;
    String str_a() default "";
}
class FieldGetDeclaredAnnotations1 {
    @interface1(num = 333, str = "test1")
    public int num;
    @interface1(num = 333, str = "test1")
    public String str;
}
class FieldGetDeclaredAnnotations1_a extends FieldGetDeclaredAnnotations1 {
    @interface1_a(num_a = 666, str_a = "right1")
    public int num;
    @interface1_a(num_a = 666, str_a = "right1")
    public String str;
}
public class RTFieldGetDeclaredAnnotations1 {
    public static void main(String[] args) {
        try {
            Class cls = Class.forName("FieldGetDeclaredAnnotations1_a");
            Field instance1 = cls.getField("num");
            if (instance1.getDeclaredAnnotations().length == 1) {
                Annotation[] j = instance1.getDeclaredAnnotations();
                if (j[0].toString().indexOf("num_a=666") != -1 && j[0].toString().indexOf("str_a=right1") != -1) {
                    Field instance2 = cls.getDeclaredField("str");
                    if (instance2.getDeclaredAnnotations().length == 1) {
                        Annotation[] k = instance2.getDeclaredAnnotations();
                        if (k[0].toString().indexOf("num_a=666") != -1 && k[0].toString().indexOf("str_a=right1") != -1)
                        {
                            System.out.println(0);
                        }
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
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
