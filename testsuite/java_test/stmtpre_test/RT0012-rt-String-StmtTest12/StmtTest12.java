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
    String t() default " ";
}
@Zzz1(i = 333, t = "getAnnotation")
class GetAnnotation1_a {
}
class GetAnnotation1_b {
}
public class StmtTest12 {
    public static void main(String[] args) {
        try {
            Class clazz1 = Class.forName("StmtTest12"); // 3
            Class clazz2 = Class.forName("StmtTest12");
            for (int ii = 0; ii < 100; ii++) {
                clazz1 = Class.forName("GetAnnotation1_a"); // 4
                clazz2 = Class.forName("GetAnnotation1_b"); // 5
            }
            if (clazz1.getAnnotation(Zzz1.class).toString().indexOf("t=getAnnotation") != -1 // 1
                    && clazz1.getAnnotation(Zzz1.class).toString().indexOf("i=333") != -1  // 2
                    && clazz2.getAnnotation(Zzz1.class) == null) { // 7
                System.out.println(0);
            }
        } catch (ClassNotFoundException e) {
            System.out.println(2);
        }
    }
}