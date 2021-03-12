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
import java.lang.reflect.AccessibleObject;
public class ConstructorExAccessibleObjectisAnnotationPresentNullPointerException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        result = ConstructorExAccessibleObjectisAnnotationPresentNullPointerExceptionTest_1();
        ConstructorExAccessibleObjectisAnnotationPresentNullPointerExceptionTest_2();
        if (result == 4 && ConstructorExAccessibleObjectisAnnotationPresentNullPointerException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int ConstructorExAccessibleObjectisAnnotationPresentNullPointerExceptionTest_1() {
        int result1 = 4;
        try {
            // boolean isAnnotationPresent(Class<? extends Annotation> annotationClass)
            AccessibleObject sampleconstructor = SampleClass7.class.getConstructor(String.class);
            boolean b = sampleconstructor.isAnnotationPresent(CustomAnnotations7.class);
            //System.out.println(b);
            if (b) {
                ConstructorExAccessibleObjectisAnnotationPresentNullPointerException.res = ConstructorExAccessibleObjectisAnnotationPresentNullPointerException.res - 2;
            }
        } catch (NoSuchMethodException | SecurityException | NullPointerException e) {
            e.printStackTrace();
        }
        return result1;
    }
    private static int ConstructorExAccessibleObjectisAnnotationPresentNullPointerExceptionTest_2() {
        int result1 = 4;
        try {
            // boolean isAnnotationPresent(Class<? extends Annotation> annotationClass)
            AccessibleObject sampleconstructor = SampleClass7.class.getConstructor(String.class);
            sampleconstructor.isAnnotationPresent(null);
            //System.out.println(b);
        } catch (NoSuchMethodException | SecurityException | NullPointerException e) {
            ConstructorExAccessibleObjectisAnnotationPresentNullPointerException.res = ConstructorExAccessibleObjectisAnnotationPresentNullPointerException.res - 2;
        }
        return result1;
    }
}
@CustomAnnotations7(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass7 {
    private String sampleField;
    @CustomAnnotations7(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass7(String s) {
    }
    public String getSampleField() {
        return sampleField;
    }
    public void setSampleField(String sampleField) {
        this.sampleField = sampleField;
    }
}
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE, ElementType.METHOD, ElementType.CONSTRUCTOR})
@interface CustomAnnotations7 {
    String name();
    String value();
}
