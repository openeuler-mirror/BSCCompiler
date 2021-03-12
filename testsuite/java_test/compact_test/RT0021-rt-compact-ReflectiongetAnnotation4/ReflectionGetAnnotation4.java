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
import java.lang.reflect.Method;
@Target(ElementType.METHOD)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
@interface Zzz4 {
    int i() default 0;
    String t() default "";
}
@interface Zzz4_a {
int ii() default 0;
String tt() default "";
}
class GetAnnotation4_a {
    @Zzz4(i = 333, t = "getAnnotation")
    public void qqq() {
    };
    @Zzz4_a(ii = 555, tt = "test")
    public void rrr() {
    };
}
public class ReflectionGetAnnotation4 {
    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotation4_a");
            Method zhu1 = zqp1.getMethod("qqq");
            Method zhu2 = zqp1.getMethod("rrr");
            if (zhu1.getAnnotation(Zzz4_a.class) == null && zhu2.getAnnotation(Zzz4.class) == null) {
                zhu1.getAnnotation(Zzz4_a.class).ii();
                System.out.println(2);
            }
            System.out.println(2);
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            System.out.println(0);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
