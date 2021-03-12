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
import java.lang.annotation.Annotation;
import java.lang.annotation.ElementType;
import java.lang.reflect.Field;
public class FieldExAccessibleObjectgetAnnotations {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(new FieldExAccessibleObjectgetAnnotations().run());
    }
    /**
     * main test fun
     * @return status code
    */

    public int run() {
        int result = 2; /*STATUS_FAILED*/
        try {
            result = fieldExAccessibleObjectgetAnnotations();
        } catch (Exception e) {
            FieldExAccessibleObjectgetAnnotations.res = FieldExAccessibleObjectgetAnnotations.res - 20;
        }
        if (result == 4 && FieldExAccessibleObjectgetAnnotations.res == 89) {
            result = 0;
        }
        return result;
    }
    private int fieldExAccessibleObjectgetAnnotations() throws NoSuchFieldException, SecurityException {
        //   Annotation[] getAnnotations()
        int result1 = 4; /*STATUS_FAILED*/
        Field field = SampleClassField4.class.getDeclaredField("id");
        try {
            Annotation[] tp = field.getAnnotations();
            if (tp[0].toString().equals("@FieldAnnotations(name=id, value=1)")) {
                FieldExAccessibleObjectgetAnnotations.res = FieldExAccessibleObjectgetAnnotations.res - 10;
            } else {
                System.out.println("Failed not as expect");
                FieldExAccessibleObjectgetAnnotations.res = FieldExAccessibleObjectgetAnnotations.res - 15;
            }
        } catch (NullPointerException e) {
            FieldExAccessibleObjectgetAnnotations.res = FieldExAccessibleObjectgetAnnotations.res - 15;
        }
        return result1;
    }
}
class SampleClassField4 {
    @FieldAnnotations(name = "id", value = "1")
    String id;
    String name;
}
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@interface FieldAnnotations {
    String name();
    String value();
}