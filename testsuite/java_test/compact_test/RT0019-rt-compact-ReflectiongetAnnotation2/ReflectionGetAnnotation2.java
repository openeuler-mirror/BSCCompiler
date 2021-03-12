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
@interface IF2 {
    int i() default 0;
    String t() default "";
}
class GetAnnotation2_a {
    @IF2(i = 333, t = "getAnnotation")
    public int i_a;
    public String t_a;
}
class GetAnnotation2_b {
    public int i_b;
    public String t_b;
}
public class ReflectionGetAnnotation2 {
    public static void main(String[] args) {
        try {
            Class cls1 = Class.forName("GetAnnotation2_a");
            Class cls2 = Class.forName("GetAnnotation2_b");
            Field instance1 = cls1.getField("i_a");
            Field instance2 = cls1.getField("t_a");
            Field instance3 = cls2.getField("i_b");
            if (instance1.getAnnotation(IF2.class).i() == 333 && instance1.getAnnotation(IF2.class).
                    t().equals("getAnnotation") && instance2.getAnnotation(IF2.class) == null &&
                    instance3.getAnnotation(IF2.class) == null) {
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
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
