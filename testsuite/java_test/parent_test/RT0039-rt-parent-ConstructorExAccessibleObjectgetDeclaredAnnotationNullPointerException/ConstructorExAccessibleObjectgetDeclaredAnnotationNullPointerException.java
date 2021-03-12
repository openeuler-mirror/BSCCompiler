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
public class ConstructorExAccessibleObjectgetDeclaredAnnotationNullPointerException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = ConstructorExAccessiblegetDeclaredAnnotationNullPointerExceptionTest_1();
            ConstructorExAccessiblegetDeclaredAnnotationNullPointerExceptionTest_2();
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (result == 4 && ConstructorExAccessibleObjectgetDeclaredAnnotationNullPointerException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int ConstructorExAccessiblegetDeclaredAnnotationNullPointerExceptionTest_2() {
        int result1 = 4;
        try {
            AccessibleObject sampleconstructor = SampleClass3.class.getConstructor(String.class);
            sampleconstructor.getDeclaredAnnotation(null);
            //System.out.println(a.toString());
        } catch (NullPointerException e) {
            //e.printStackTrace();
            ConstructorExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = ConstructorExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 2;
        } catch (NoSuchMethodException | SecurityException e) {
            //e.printStackTrace();
        }
        return result1;
    }
    private static int ConstructorExAccessiblegetDeclaredAnnotationNullPointerExceptionTest_1() {
        int result1 = 4;
        try {
            //T getDeclaredAnnotation(Class<T> annotationClass)
            AccessibleObject sampleconstructor = SampleClass3.class.getConstructor(String.class);
            CustomAnnotations3 a = sampleconstructor.getDeclaredAnnotation(CustomAnnotations3.class);
            //System.out.println(a.toString());
            if (a.toString().equals("@CustomAnnotations3(name=sampleConstructor, value=Sample Constructor Annotation)")) {
                ConstructorExAccessibleObjectgetDeclaredAnnotationNullPointerException.res = ConstructorExAccessibleObjectgetDeclaredAnnotationNullPointerException.res - 2;
            }
        } catch (NullPointerException | NoSuchMethodException | SecurityException e) {
            //e.printStackTrace();
        }
        return result1;
    }
}
@CustomAnnotations3(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass3 {
    private String sampleField;
    @CustomAnnotations3(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass3(String s) {
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
@interface CustomAnnotations3 {
    String name();
    String value();
}
