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
@interface Zzz3 {
    int i() default 0;
    String t() default "";
}
class GetAnnotation3_a {
    @Zzz3(i = 333, t = "getAnnotation")
    public void qqq() {
    };
    public void rrr() {
    };
}
class GetAnnotation3_b {
    public void www() {
    };
}
public class ReflectionGetAnnotation3 {
    public static void main(String[] args) {
        try {
            Class zqp1 = Class.forName("GetAnnotation3_a");
            Method zhu1 = zqp1.getMethod("qqq");
            Method zhu2 = zqp1.getMethod("rrr");
            zhu1.getAnnotation(Zzz3.class).t();
            zhu2.getAnnotation(Zzz3.class).i();
            System.out.println(2);
        } catch (ClassNotFoundException e) {
            System.err.println(e);
            System.out.println(2);
        } catch (NoSuchMethodException e1) {
            System.err.println(e1);
            System.out.println(2);
        } catch (NullPointerException e2) {
            try {
                Class zqp2 = Class.forName("GetAnnotation3_b");
                Method zhu3 = zqp2.getMethod("www");
                zhu3.getAnnotation(Zzz3.class).i();
                System.out.println(2);
            } catch (ClassNotFoundException e3) {
                System.err.println(e3);
                System.out.println(2);
            } catch (NoSuchMethodException e4) {
                System.err.println(e4);
                System.out.println(2);
            } catch (NullPointerException e5){
                System.out.println(0);
            }
        }
    }
}