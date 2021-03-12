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
public class ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerExceptionTest_1();
            ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerExceptionTest_2();
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (result == 4 && ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerExceptionTest_1() {
        int result1 = 4;
        AccessibleObject sampleconstructor;
        try {
            sampleconstructor = SampleClass5.class.getConstructor(String.class);
            CustomAnnotations5[] a = sampleconstructor.getDeclaredAnnotationsByType(CustomAnnotations5.class);
            //System.out.println(a.getClass());
            if (a.length == 1 && a.getClass().toString().equals("class [LCustomAnnotations5;")) {
                ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 2;
            }
        } catch (NoSuchMethodException | SecurityException | NullPointerException e) {
            e.printStackTrace();
        }
        return result1;
    }
    private static int ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerExceptionTest_2() {
        int result1 = 4;
        AccessibleObject sampleconstructor;
        try {
            sampleconstructor = SampleClass5.class.getConstructor(String.class);
            sampleconstructor.getDeclaredAnnotationsByType(null);
            //System.out.println(a.getClass());
        } catch (NoSuchMethodException | SecurityException | NullPointerException e) {
            //e.printStackTrace();
            ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res = ConstructorExAccessibleObjectgetDeclaredAnnotationsByTypeNullPointerException.res - 2;
        }
        return result1;
    }
}
@CustomAnnotations5(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass5 {
    private String sampleField;
    @CustomAnnotations5(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass5(String s) {
        this.sampleField = s;
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
@interface CustomAnnotations5 {
    String name();
    String value();
}
