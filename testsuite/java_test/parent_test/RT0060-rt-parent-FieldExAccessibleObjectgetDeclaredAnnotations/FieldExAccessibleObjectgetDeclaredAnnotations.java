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
import java.lang.annotation.Annotation;
import java.lang.annotation.Repeatable;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectgetDeclaredAnnotations {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new FieldExAccessibleObjectgetDeclaredAnnotations().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectgetDeclaredAnnotations1();
        } catch (Exception e) {
            FieldExAccessibleObjectgetDeclaredAnnotations.res = FieldExAccessibleObjectgetDeclaredAnnotations.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectgetDeclaredAnnotations.res == 89) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectgetDeclaredAnnotations1() throws NoSuchFieldException, SecurityException {
        //    Annotation[] getDeclaredAnnotations()
        int result1 = 4; /*STATUS_FAILED*/
        Field field = SampleClassField5.class.getDeclaredField("id");
        try {
            Annotation[] tp = field.getDeclaredAnnotations();
            if (tp[0].toString().equals("@CustomAnnotation6(value=id)")) {
                FieldExAccessibleObjectgetDeclaredAnnotations.res = FieldExAccessibleObjectgetDeclaredAnnotations.res - 10;
            } else {
                System.out.println("Failed not as expect");
                FieldExAccessibleObjectgetDeclaredAnnotations.res = FieldExAccessibleObjectgetDeclaredAnnotations.res - 15;
            }
        } catch (NullPointerException e) {
            FieldExAccessibleObjectgetDeclaredAnnotations.res = FieldExAccessibleObjectgetDeclaredAnnotations.res - 15;
        }
        return result1;
    }
}
@CustomAnnotation6(value = "class")
class SampleClassField5 {
    @CustomAnnotation6(value = "id")
    String id;
    String name;
}
@Target({ElementType.TYPE, ElementType.FIELD, ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@Repeatable(CustomAnnotation7.class)
@interface CustomAnnotation6 {
    String value();
}
@Target({ElementType.TYPE, ElementType.FIELD, ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@interface CustomAnnotation7 {
    CustomAnnotation6[] value();
}
