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
public class ConstructorExExecutablegetAnnotationsByTypeNullPointerException {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = constructorExExecutablegetAnnotationsByTypeNPExceptionTest1();
            constructorExExecutablegetAnnotationsByTypeNPExceptionTest1();
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (result == 4 && ConstructorExExecutablegetAnnotationsByTypeNullPointerException.res == 95) {
            result = 0;
        }
        return result;
    }
    private static int constructorExExecutablegetAnnotationsByTypeNPExceptionTest1() {
        int result1 = 4;
        Executable sampleExecutable;
        try {
            // <T extends Annotation> T[] getAnnotationsByType(Class<T> annotationClass)
            sampleExecutable = SampleClass10.class.getConstructor(String.class);
            CustomAnnotations10[] annotations10s = sampleExecutable.getDeclaredAnnotationsByType(CustomAnnotations10.class);
            // System.out.println(a.getClass());
            if (annotations10s.length == 1 && annotations10s.getClass().toString().equals("class [LCustomAnnotations10;")) {
                ConstructorExExecutablegetAnnotationsByTypeNullPointerException.res = ConstructorExExecutablegetAnnotationsByTypeNullPointerException.res - 2;
            }
        } catch (NoSuchMethodException | SecurityException | NullPointerException e) {
            e.printStackTrace();
        }
        return result1;
    }
}
@CustomAnnotations10(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass10 {
    private String sampleField;
    @CustomAnnotations10(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass10(String str) {
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
@interface CustomAnnotations10 {
    String name();
    String value();
}