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
public class ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerExceptionTest_1();
            ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerExceptionTest_2();
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (result == 4 && ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerExceptionTest_1() {
        int result = 4;
        try {
            // <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
            AccessibleObject sampleconstructor = SampleClass2.class.getConstructor(String.class);
            CustomAnnotations2[] a = sampleconstructor.getAnnotationsByType(CustomAnnotations2.class);
            //System.out.println(a.toString());
            //System.out.println(a[0].toString());
            if (a.length == 1 && a.getClass().toString().equals("class [LCustomAnnotations2;")) {
                ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 2;
            }
        } catch (NullPointerException | SecurityException | NoSuchMethodException e ) {
            e.printStackTrace();
        }
        return result;
    }
    private static int ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerExceptionTest_2() {
        int result = 4;
        try {
            // <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
            AccessibleObject sampleconstructor = SampleClass2.class.getConstructor(String.class);
            sampleconstructor.getAnnotationsByType(null);
            //System.out.println(a.toString());
            //System.out.println(a[0].toString());
        } catch (SecurityException | NoSuchMethodException e) {
            //e.printStackTrace();
        } catch (NullPointerException e) {
            //e.printStackTrace();
            ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerException.res = ConstructorExAccessibleObjectgetAnnotationsByTypeNullPointerException.res - 2;
        }
        return result;
    }
}
@CustomAnnotations2(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass2 {
    private String sampleField;
    @CustomAnnotations2(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass2(String s) {
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
@interface CustomAnnotations2 {
    String name();
    String value();
}
