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


import java.lang.annotation.Target;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Retention;
import java.lang.annotation.Repeatable;
import java.lang.annotation.ElementType;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectgetAnnotationsByTypeNullPointerException1();
        } catch (Exception e) {
            FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 20;
        }
        try {
            result = fieldExAccessibleObjectgetAnnotationsByTypeNullPointerException2();
        } catch (Exception e) {
            FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res == 74) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectgetAnnotationsByTypeNullPointerException1() throws NoSuchFieldException, SecurityException {
        //   <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
        int result1 = 4; /*STATUS_FAILED*/
        Field field = SampleClassField2.class.getDeclaredField("id");
        try {
            CustomAnnotation2[] tp = field.getAnnotationsByType(CustomAnnotation2.class);
            if (tp[0].toString().equals("@CustomAnnotation2(value=id)")) {
                FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 10;
            } else {
                System.out.println("Failed not as expect");
                FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 15;
            }
        } catch (NullPointerException e) {
            FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 15;
        }
        return result1;
    }
    private int fieldExAccessibleObjectgetAnnotationsByTypeNullPointerException2() throws NoSuchFieldException, SecurityException {
        // NullPointerException - if the given annotation class is null
        // <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
        int result1 = 4; /*STATUS_FAILED*/
        Field field = SampleClassField2.class.getDeclaredField("id");
        try {
            CustomAnnotation2[] tp = field.getAnnotationsByType(null);
            FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 10;
        } catch (NullPointerException e) {
            FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 15;
        }
        return result1;
    }
}
@CustomAnnotation2(value = "class")
class SampleClassField2 {
    @CustomAnnotation2(value = "id")
    String id;
    String name;
}
@Target({ElementType.TYPE, ElementType.FIELD, ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@Repeatable(CustomAnnotation3.class)
@interface CustomAnnotation2 {
    String value();
}
@Target({ElementType.TYPE, ElementType.FIELD, ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@interface CustomAnnotation3 {
    CustomAnnotation2[] value();
}
