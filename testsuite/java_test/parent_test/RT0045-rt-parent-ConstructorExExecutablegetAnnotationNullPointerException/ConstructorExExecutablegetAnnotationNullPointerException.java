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
import java.lang.reflect.Executable;
public class ConstructorExExecutablegetAnnotationNullPointerException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = constructorExExecutablegetAnnotationNullPointerExceptionTest1();
            constructorExExecutablegetAnnotationNullPointerExceptionTest2();
        } catch (Exception e) {
            ConstructorExExecutablegetAnnotationNullPointerException.res = ConstructorExExecutablegetAnnotationNullPointerException.res - 10;
        }
        if (result == 4 && ConstructorExExecutablegetAnnotationNullPointerException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int constructorExExecutablegetAnnotationNullPointerExceptionTest1() {
        int result1 = 4;
        try {
            // <T extends Annotation> T getAnnotation(Class<T> annotationClass)
            Executable sampleexecutable = SampleClass9.class.getConstructor(String.class);
            CustomAnnotations9 annotation = sampleexecutable.getAnnotation(CustomAnnotations9.class);
            if (annotation.toString().equals("@CustomAnnotations9(name=sampleConstructor, value=Sample Constructor Annotation)")) {
                ConstructorExExecutablegetAnnotationNullPointerException.res = ConstructorExExecutablegetAnnotationNullPointerException.res - 2;
            }
        } catch (NullPointerException | NoSuchMethodException | SecurityException e) {
            e.printStackTrace();
        }
        return result1;
    }
    private static int constructorExExecutablegetAnnotationNullPointerExceptionTest2() {
        int result1 = 4;
        try {
            // <T extends Annotation> T getAnnotation(Class<T> annotationClass)
            Executable sampleexecutable = SampleClass9.class.getConstructor(String.class);
            sampleexecutable.getAnnotation(null);
        } catch (NullPointerException e) {
            ConstructorExExecutablegetAnnotationNullPointerException.res = ConstructorExExecutablegetAnnotationNullPointerException.res - 2;
        } catch (NoSuchMethodException | SecurityException e1) {
            String eMsg = e1.toString();
        }
        return result1;
    }
}
@CustomAnnotations9(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass9 {
    private String sampleField;
    @CustomAnnotations9(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass9(String str) {
        this.sampleField = str;
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
@interface CustomAnnotations9 {
    String name();
    String value();
}
