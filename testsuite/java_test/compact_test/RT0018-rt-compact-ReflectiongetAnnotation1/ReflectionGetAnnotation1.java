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
@interface Zzz1 {
int i() default 0;
String t() default "";
}
@Zzz1(i = 333, t = "getAnnotation")
class GetAnnotation1_a {
}
class GetAnnotation1_b {
}
public class ReflectionGetAnnotation1 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("GetAnnotation1_a");
            Class clazz2 = Class.forName("GetAnnotation1_b");
            if (clazz1.getAnnotation(Zzz1.class).toString().indexOf("t=getAnnotation") != -1
                    && clazz1.getAnnotation(Zzz1.class).toString().indexOf("i=333") != -1
                    && clazz2.getAnnotation(Zzz1.class) == null) {
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
