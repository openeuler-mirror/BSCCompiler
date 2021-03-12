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
import java.lang.reflect.AccessibleObject;
public class ConstructorExAccessibleObjectgetAnnotations {
    static int res = 99;
    static String eMsg;
    public static void main(String argv[]) {
        System.out.println(run());
    }
    static int run() {
        int result = 2;
        try {
            result = constructorExObjectgetAnnotationsTest1();
        } catch (Exception e) {
            eMsg = e.toString();
        }
        if (result == 4 && ConstructorExAccessibleObjectgetAnnotations.res == 97) {
            result = 0;
        }
        return result;
    }
    private static int constructorExObjectgetAnnotationsTest1() {
        int result1 = 4;
        try {
            // Annotation[] getAnnotations()
            AccessibleObject sampleconstructor = SampleClass0.class.getConstructor(String.class);
            Annotation[] annotations = sampleconstructor.getAnnotations();
            if (annotations.length == 1 && annotations.getClass().toString().equals("class [Ljava.lang.annotation.Annotation;")) {
                ConstructorExAccessibleObjectgetAnnotations.res = ConstructorExAccessibleObjectgetAnnotations.res - 2;
            }
        } catch (NoSuchMethodException | SecurityException e1) {
            eMsg = e1.toString();
        }
        return result1;
    }
}
@CustomAnnotations0(name = "SampleClass", value = "Sample Class Annotation")
class SampleClass0 {
    private String sampleField;
    @CustomAnnotations0(name = "sampleConstructor", value = "Sample Constructor Annotation")
    public SampleClass0(String s) {
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
@interface CustomAnnotations0 {
    String name();
    String value();
}
