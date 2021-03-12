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


import java.lang.annotation.Annotation;
public class ClassGetDeclaredAnnotationNPE {
    public static void main(String argv[]) {
        int result = 2; /* STATUS_FAILED*/

        try {
            result = ClassGetDeclaredAnnotationNPE_1();
        } catch (Exception e) {
            result = 3;
        }
        System.out.println(result);
    }
    public static int ClassGetDeclaredAnnotationNPE_1() {
        try {
            Annotation a = MyClass.class.getDeclaredAnnotation(Deprecated.class);
            a = MyClass.class.getDeclaredAnnotation(null);
        } catch (NullPointerException e) {
            return 0;
        }
        return 4;
    }
    @Deprecated
    class MyClass extends MySuperClass {
    }
    @MyAnnotation
    class MySuperClass {
    }
    @interface MyAnnotation {
    }
}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full 0\n
