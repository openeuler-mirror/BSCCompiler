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
import java.lang.annotation.Target;
import java.lang.annotation.ElementType;
import java.lang.annotation.RetentionPolicy;
import java.lang.reflect.AccessibleObject;
import java.lang.Thread;
public class ConstructorExAccessibleObjectgetAnnotationNullPointerException {
    static int res = 99;
    static String eMsg;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = constructorExObjectgetAnnotationNullPointerExceptionTest1();
            constructorExObjectgetAnnotationNullPointerExceptionTest2();
        } catch (Exception e) {
            ConstructorExAccessibleObjectgetAnnotationNullPointerException.res = ConstructorExAccessibleObjectgetAnnotationNullPointerException.res - 10;
        }
        if (result == 4 && ConstructorExAccessibleObjectgetAnnotationNullPointerException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int constructorExObjectgetAnnotationNullPointerExceptionTest1() {
        int result1 = 4;
        try {
            // <T extends Annotation> T getAnnotation(Class<T> annotationClass)
            AccessibleObject sampleconstructor = SampleClass9q.class.getConstructor(String.class);
            // System.out.println(sampleconstructor.getClass());
            CustomAnnotations1 annotation = sampleconstructor.getAnnotation(CustomAnnotations1.class);
            if (annotation.toString().equals("@CustomAnnotations1(name=sampleConstructor, value=Sample Constructor Annotation)")) {
                ConstructorExAccessibleObjectgetAnnotationNullPointerException.res = ConstructorExAccessibleObjectgetAnnotationNullPointerException.res - 2;
            }
        } catch (NullPointerException e) {
            eMsg = e.toString();
        } catch (NoSuchMethodException | SecurityException e1) {
            eMsg = e1.toString();
        }
        return result1;
    }
    private static int constructorExObjectgetAnnotationNullPointerExceptionTest2() {
        int result1 = 4;
        try {
            // <T extends Annotation> T getAnnotation(Class<T> annotationClass)
            AccessibleObject sampleconstructor = SampleClass9q.class.getConstructor(String.class);
            // System.out.println(sampleconstructor.getClass());
            sampleconstructor.getAnnotation(null);
            // System.out.println(a.toString());
        } catch (NullPointerException e) {
            ConstructorExAccessibleObjectgetAnnotationNullPointerException.res = ConstructorExAccessibleObjectgetAnnotationNullPointerException.res - 2;
        } catch (NoSuchMethodException | SecurityException e1) {
            eMsg = e1.toString();
        }
        return result1;
    }
}
@CustomAnnotations1(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass9q {
    private String sampleField;
    @CustomAnnotations1(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass9q(String s) {
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
@interface CustomAnnotations1 {
    String name();
    String value();
}
