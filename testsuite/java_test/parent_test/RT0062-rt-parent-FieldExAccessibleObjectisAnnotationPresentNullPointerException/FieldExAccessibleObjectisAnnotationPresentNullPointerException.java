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


import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectisAnnotationPresentNullPointerException {
    static int res = 99;
    public static void main(String[] args) {
        System.out.println(new FieldExAccessibleObjectisAnnotationPresentNullPointerException().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectisAnnotationPresentNullPointerException1();
        } catch (Exception e) {
            FieldExAccessibleObjectisAnnotationPresentNullPointerException.res = FieldExAccessibleObjectisAnnotationPresentNullPointerException.res - 20;
        }
        try {
            result = fieldExAccessibleObjectisAnnotationPresentNullPointerException2();
        } catch (Exception e) {
            FieldExAccessibleObjectisAnnotationPresentNullPointerException.res = FieldExAccessibleObjectisAnnotationPresentNullPointerException.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectisAnnotationPresentNullPointerException.res == 74) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectisAnnotationPresentNullPointerException1() throws NoSuchFieldException {
        //  boolean isAnnotationPresent(Class<? extends Annotation> annotationClass)
        int result1 = 4;
        Field f1 = SampleClassFieldH3.class.getDeclaredField("id");
        try {
            if (f1.isAnnotationPresent(CustomAnnotationsH3.class)) {
                FieldExAccessibleObjectisAnnotationPresentNullPointerException.res = FieldExAccessibleObjectisAnnotationPresentNullPointerException.res - 10;
            } else {
                FieldExAccessibleObjectisAnnotationPresentNullPointerException.res = FieldExAccessibleObjectisAnnotationPresentNullPointerException.res - 15;
            }
        } catch (Exception e) {
            FieldExAccessibleObjectisAnnotationPresentNullPointerException.res = FieldExAccessibleObjectisAnnotationPresentNullPointerException.res - 15;
        }
        return result1;
    }
    private int fieldExAccessibleObjectisAnnotationPresentNullPointerException2() throws NoSuchFieldException {
        int result2 = 4;
        // NullPointerException - if the given annotation class is null
        // boolean isAnnotationPresent(Class<? extends Annotation> annotationClass)
        Field f1 = SampleClassFieldH3.class.getDeclaredField("id");
        try {
            boolean tp = f1.isAnnotationPresent(null);
            FieldExAccessibleObjectisAnnotationPresentNullPointerException.res = FieldExAccessibleObjectisAnnotationPresentNullPointerException.res - 10;
        } catch (NullPointerException e) {
            FieldExAccessibleObjectisAnnotationPresentNullPointerException.res = FieldExAccessibleObjectisAnnotationPresentNullPointerException.res - 15;
        }
        return result2;
    }
}
class SampleClassFieldH3 {
    @CustomAnnotationsH3(name = "id")
    String id;
}
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@interface CustomAnnotationsH3 {
    String name();
}
