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


import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.annotation.ElementType;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException {
    static int res = 99;
    public static void main(String[] argv) {
        System.out.println(new FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException().run());
    }
    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = FieldExAccessibleObjectgetDeclaredAnnotationsByType_1();
        } catch (Exception e) {
            FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 20;
        }
        try {
            result = FieldExAccessibleObjectgetDeclaredAnnotationsByType_2();
        } catch (Exception e) {
            FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res == 74) {
            result = 0;
        }
        return result;
    }
    private int FieldExAccessibleObjectgetDeclaredAnnotationsByType_1() throws NoSuchFieldException {
        //  <T extends Annotation> T[] getDeclaredAnnotationsByType(Class<T> annotationClass)
        int result1 = 4;
        Field f1 = SampleClassField_h1.class.getDeclaredField("id");
        try {
            CustomAnnotations_h1[] tp = f1.getDeclaredAnnotationsByType(CustomAnnotations_h1.class);
//            System.out.println(tp[0]);
            if (tp[0].toString().equals("@CustomAnnotations_h1(name=id)")) {
                FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 10;
            } else {
                System.out.println("Failed not as expect");
                FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 15;
            }
        } catch (Exception e) {
            FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 15;
        }
        return result1;
    }
    private int FieldExAccessibleObjectgetDeclaredAnnotationsByType_2() throws NoSuchFieldException {
        //  NullPointerException - if the given annotation class is null
        //  <T extends Annotation> T[] getDeclaredAnnotationsByType(Class<T> annotationClass)
        int result2 = 4;
        Field f1 = SampleClassField_h1.class.getDeclaredField("id");
        try {
            CustomAnnotations_h1[] tp = f1.getDeclaredAnnotationsByType(null);
            FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 10;
        } catch (NullPointerException e) {
            FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = FieldExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 15;
        }
        return result2;
    }
}
class SampleClassField_h1 {
    @CustomAnnotations_h1(name = "id")
    String id;
}
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@interface CustomAnnotations_h1 {
    String name();
}
