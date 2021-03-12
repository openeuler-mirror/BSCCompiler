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


import java.lang.annotation.Annotation;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.Executable;
public class ConstructorExExecutablegetDeclaredAnnotations {
    static int res = 99;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = constructorExExecutablegetDeclaredAnnotations1();
        } catch (Exception e) {
            e.printStackTrace();
        }
        if (result == 4 && ConstructorExExecutablegetDeclaredAnnotations.res == 97) {
            result = 0;
        }
        return result;
    }
    private static int constructorExExecutablegetDeclaredAnnotations1() {
        int result1 = 4;
        try {
            Executable sampleExecutable = SampleClass11.class.getConstructor(String.class);
            Annotation[] annotations = sampleExecutable.getDeclaredAnnotations();
            if (annotations.length == 1 && annotations.getClass().toString().equals("class [Ljava.lang.annotation.Annotation;")) {
                ConstructorExExecutablegetDeclaredAnnotations.res = ConstructorExExecutablegetDeclaredAnnotations.res - 2;
            }
        } catch (NoSuchMethodException | SecurityException e1) {
            e1.printStackTrace();
        }
        return result1;
    }
}
@CustomAnnotations11(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass11 {
    private String sampleField;
    @CustomAnnotations11(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass11(String s) {
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
@interface CustomAnnotations11 {
    String name();
    String value();
}