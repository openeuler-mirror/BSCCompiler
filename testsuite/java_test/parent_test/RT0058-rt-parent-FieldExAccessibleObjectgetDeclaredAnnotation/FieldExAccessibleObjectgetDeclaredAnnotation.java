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
import java.lang.annotation.Repeatable;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectgetDeclaredAnnotation {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new FieldExAccessibleObjectgetDeclaredAnnotation().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectgetDeclaredAnnotation1();
        } catch (Exception e) {
            FieldExAccessibleObjectgetDeclaredAnnotation.res = FieldExAccessibleObjectgetDeclaredAnnotation.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectgetDeclaredAnnotation.res == 89) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectgetDeclaredAnnotation1() throws NoSuchFieldException, SecurityException {
        //    <T extends Annotation> T getDeclaredAnnotation(Class<T> annotationClass)
        int result1 = 4; /*STATUS_FAILED*/
        Field field = SampleClassField3.class.getDeclaredField("id");
        try {
            CustomAnnotation4 tp = field.getDeclaredAnnotation(CustomAnnotation4.class);
            if (tp.toString().equals("@CustomAnnotation4(value=id)")) {
                FieldExAccessibleObjectgetDeclaredAnnotation.res = FieldExAccessibleObjectgetDeclaredAnnotation.res - 10;
            } else {
                System.out.println("Failed not as expect");
                FieldExAccessibleObjectgetDeclaredAnnotation.res = FieldExAccessibleObjectgetDeclaredAnnotation.res - 15;
            }
        } catch (NullPointerException e) {
            FieldExAccessibleObjectgetDeclaredAnnotation.res = FieldExAccessibleObjectgetDeclaredAnnotation.res - 15;
        }
        return result1;
    }
}
@CustomAnnotation4(value = "class")
class SampleClassField3 {
    @CustomAnnotation4(value = "id")
    String id;
    String name;
}
@Target({ElementType.TYPE, ElementType.FIELD, ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@Repeatable(CustomAnnotation5.class)
@interface CustomAnnotation4 {
    String value();
}
@Target({ElementType.TYPE, ElementType.FIELD, ElementType.METHOD})
@Retention(RetentionPolicy.RUNTIME)
@interface CustomAnnotation5 {
    CustomAnnotation4[] value();
}